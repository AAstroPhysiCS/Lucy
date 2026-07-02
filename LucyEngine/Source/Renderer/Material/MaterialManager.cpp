#include "lypch.h"
#include "MaterialManager.h"
#include "PBRMaterial.h"

#include "Renderer/Pipeline/GraphicsPipeline.h"

#include "Core/FileSystem.h"
#include "assimp/material.h"

namespace Lucy {

	MaterialManager::MaterialManager(const Unique<PipelineManager>& pipelineManager)
		: m_PipelineManager(pipelineManager) {
	}

	MaterialID MaterialManager::CreateMaterialByPath(MaterialType materialType, aiMaterial* aiMaterial, const std::string& importedFilePath) {
		switch (materialType) {
			case MaterialType::PBR:
				return CreatePBRMaterial(aiMaterial, importedFilePath);
			default:
				LUCY_ASSERT(false, "Unknown material type!");
				break;
		}
		return 0;
	}

	void MaterialManager::RTDestroyMaterial(MaterialID materialID) {
		if (m_Materials.find(materialID) == m_Materials.end())
			return;
		s_MaterialIDProvider.ReturnID(materialID);
		m_Materials.at(materialID)->RTDestroyResource();
		m_Materials.erase(materialID);
	}

	void MaterialManager::RTDestroyMaterials(const std::vector<MaterialID>& materialIDs) {
		for (MaterialID materialID : materialIDs)
			RTDestroyMaterial(materialID);
	}

	void MaterialManager::DestroyAll() {
		for (const auto& [id, material] : m_Materials)
			material->RTDestroyResource();
		m_Materials.clear();
		s_MaterialIDProvider.Reset();
	}

	void MaterialManager::UpdateMaterialsIfNecessary() {
		LUCY_PROFILE_NEW_EVENT("MaterialManager::UpdateMaterialsIfNecessary");
		for (const auto& [id, material] : m_Materials)
			material->Update();
	}

	MaterialID MaterialManager::CreatePBRMaterial(aiMaterial* aiMaterial, const std::string& importedFilePath) {
		static constexpr const float NOT_SET_MATERIAL_PROPERTY = -1.0f;

		aiColor3D diffuse{ 1.0f };
		float metallic = NOT_SET_MATERIAL_PROPERTY;
		float roughness = NOT_SET_MATERIAL_PROPERTY;
		float aoContribution = 1.0f;

		int32_t shadingModel = aiShadingMode_Blinn;
		aiMaterial->Get(AI_MATKEY_SHADING_MODEL, shadingModel);

		switch (shadingModel) {
			case aiShadingMode_Phong:
			case aiShadingMode_Blinn: {
				aiColor3D specularColor{ 1.0f };
				aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
				aiMaterial->Get(AI_MATKEY_SPECULAR_FACTOR, specularColor);
				aiMaterial->Get(AI_MATKEY_SHININESS, roughness);
				// https://computergraphics.stackexchange.com/questions/1515/what-is-the-accepted-method-of-converting-shininess-to-roughness-and-vice-versa
				roughness = sqrt(2 / (roughness + 2));
				float specIntensity = std::max(specularColor.r,
					std::max(specularColor.g, specularColor.b));
				metallic = glm::clamp((specIntensity - 0.04f) / (1.0f - 0.04f), 0.0f, 1.0f);
				break;
			}
			case aiShadingMode_OrenNayar:
			case aiShadingMode_PBR_BRDF: {
				aiMaterial->Get(AI_MATKEY_BASE_COLOR, diffuse);
				aiMaterial->Get(AI_MATKEY_METALLIC_FACTOR, metallic);
				aiMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
				if (metallic == NOT_SET_MATERIAL_PROPERTY || roughness == NOT_SET_MATERIAL_PROPERTY) {
					aiMaterial->Get(AI_MATKEY_SPECULAR_FACTOR, metallic);
					aiMaterial->Get(AI_MATKEY_GLOSSINESS_FACTOR, roughness);
					roughness = 1.0f - roughness;
				}
				break;
			}
			default: 
				LUCY_ASSERT(false, "Unsupported shading model for PBR material!");
				break;
		}

		roughness = glm::clamp(roughness, 0.0f, 1.0f);
		metallic = glm::clamp(metallic, 0.0f, 1.0f);

		MaterialCreateInfo createInfo;
		createInfo.MaterialID = s_MaterialIDProvider.RequestID();
		createInfo.MaterialType = MaterialType::PBR;
		createInfo.Pipeline = m_PipelineManager->GetAs<Pipeline>("PBRGeometryPipeline");
		
		PBRMaterialData pbrMaterialData;
		pbrMaterialData.Albedo = glm::vec3(diffuse.r, diffuse.g, diffuse.b);
		pbrMaterialData.Metallic = metallic;
		pbrMaterialData.Roughness = roughness;
		pbrMaterialData.AOContribution = aoContribution;

		const auto& pbrMaterial = Memory::CreateRef<PBRMaterial>(createInfo, pbrMaterialData);
		LoadMaterialTextures(aiMaterial, importedFilePath, pbrMaterial);

		m_Materials.try_emplace(createInfo.MaterialID, pbrMaterial);
		return createInfo.MaterialID;
	}

	void MaterialManager::LoadMaterialTextures(aiMaterial* aiMaterial, const std::string& importedFilePath, const Ref<Material>& material) {
		const auto TryLoadTextureIntoSlot = [&](aiTextureType textureType, MaterialImageType slot, ImageFormat format) -> bool {
			aiString path;
			if (aiMaterial->GetTexture(textureType, 0, &path) != aiReturn_SUCCESS || path.length == 0)
				return false;

			auto properTexturePath = FileSystem::GetParentPath(importedFilePath) / std::string(path.C_Str());

			Renderer::EnqueueToRenderCommandQueue([material, properTexturePath, path, slot, format](const Ref<RenderDevice>& device) {
				ImageCreateInfo createInfo{};
				createInfo.Format = format;
				createInfo.ImageType = ImageType::Type2D;
				createInfo.ImageUsage = ImageUsage::AsColorAttachment;
				createInfo.GenerateSampler = true;
				createInfo.GenerateMipmap = MipmapCreateInfo::FromWidthAndHeight();
				createInfo.ImGuiUsage = true;

				RenderResourceHandle textureHandle = device->CreateImage(properTexturePath, createInfo, "PBR Image: " + std::string(path.C_Str()));
				material->SetTexture(slot, textureHandle);
			});

			return true;
		};

		switch (material->GetMaterialType()) {
			case MaterialType::PBR: {
				TryLoadTextureIntoSlot(aiTextureType_BASE_COLOR, PBRMaterial::ALBEDO_TYPE, ImageFormat::R8G8B8A8_SRGB);
				bool loadedNormal = TryLoadTextureIntoSlot(aiTextureType_NORMALS, PBRMaterial::NORMALS_TYPE, ImageFormat::R8G8B8A8_UNORM);
				if (!loadedNormal) {
					TryLoadTextureIntoSlot(aiTextureType_NORMAL_CAMERA, PBRMaterial::NORMALS_TYPE, ImageFormat::R8G8B8A8_UNORM);
				}
				TryLoadTextureIntoSlot(aiTextureType_METALNESS, PBRMaterial::METALLIC_TYPE, ImageFormat::R8G8B8A8_UNORM);
				TryLoadTextureIntoSlot(aiTextureType_DIFFUSE_ROUGHNESS, PBRMaterial::ROUGHNESS_TYPE, ImageFormat::R8G8B8A8_UNORM);
				TryLoadTextureIntoSlot(aiTextureType_AMBIENT_OCCLUSION, PBRMaterial::AO_TYPE, ImageFormat::R8G8B8A8_UNORM);
				break;
			}
			default:
				break;
		}
	}
}