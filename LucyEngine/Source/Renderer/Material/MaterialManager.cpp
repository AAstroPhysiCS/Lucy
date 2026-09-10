#include "lypch.h"

#include <numeric>

#include "MaterialManager.h"
#include "PBRMaterial.h"

#include "Renderer/Device/RenderDeviceScene.h"

#include "Renderer/Pipeline/GraphicsPipeline.h"
#include "Renderer/Image/Image.h"
#include "Renderer/Image/DDSHelper.h"

#include "Core/FileSystem.h"
#include "assimp/material.h"

namespace Lucy {

	MaterialManager::MaterialManager(const Unique<PipelineManager>& pipelineManager)
		: m_PipelineManager(pipelineManager) {
	}

	RenderDeviceObjectHandle MaterialManager::CreateMaterialByPath(MaterialType materialType, aiMaterial* aiMaterial, const std::string& importedFilePath) {
		switch (materialType) {
			case MaterialType::PBR:
				return CreatePBRMaterial(aiMaterial, importedFilePath);
			default:
				LUCY_ASSERT(false, "Unknown material type!");
				break;
		}
		return {};
	}

	void MaterialManager::RTDestroyMaterial(RenderDeviceObjectHandle handle) {
		//TODO: delete from cache
		m_Materials.at(handle)->RTDestroyResource();
		m_Materials.erase(handle);
	}

	void MaterialManager::RTDestroyMaterials(const std::vector<RenderDeviceObjectHandle>& materialIDs) {
		for (RenderDeviceObjectHandle id : materialIDs)
			RTDestroyMaterial(id);
	}

	void MaterialManager::DestroyAll() {
		for (auto& [id, material] : m_Materials) {
			material->RTDestroyResource();
		}
		m_TextureCache.clear();
		m_Materials.clear();
	}

	RenderDeviceObjectHandle MaterialManager::CreatePBRMaterial(aiMaterial* aiMaterial, const std::string& importedFilePath) {
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

				float shininess = 0.0f;
				if (aiMaterial->Get(AI_MATKEY_SHININESS, shininess) == aiReturn_SUCCESS) {
					// https://computergraphics.stackexchange.com/questions/1515/what-is-the-accepted-method-of-converting-shininess-to-roughness-and-vice-versa
					roughness = sqrt(2.0f / (shininess + 2.0f));
				} else {
					roughness = 0.8f;
				}

				// Legacy Phong materials should default to dielectric.
				metallic = 0.0f;
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

		RenderDevicePBRMaterialData materialGPUData{};
		materialGPUData.BaseColor = glm::vec4(diffuse.r, diffuse.g, diffuse.b, 1.0f);
		materialGPUData.ORME = glm::vec4(aoContribution, roughness, metallic, 0.0f);
		materialGPUData.NormalStrength = 1.0f;

		RenderDeviceObjectHandle materialDeviceHandle = Renderer::GetRenderDevice()->GetScene()->RegisterPBRMaterial(materialGPUData);

		MaterialCreateInfo createInfo;
		createInfo.MaterialType = MaterialType::PBR;
		createInfo.MaterialDeviceID = materialDeviceHandle;

		Ref<PBRMaterial> material = Memory::CreateRef<PBRMaterial>(createInfo);
		LoadMaterialTextures(aiMaterial, importedFilePath, material);

		m_Materials.try_emplace(materialDeviceHandle, material);
		return materialDeviceHandle;
	}

	void MaterialManager::LoadMaterialTextures(aiMaterial* aiMaterial, const std::string& importedFilePath, const Ref<Material>& material) {
		const auto IsValidTexture = [&aiMaterial](aiString& path, aiTextureType textureType) {
			if (aiMaterial->GetTexture(textureType, 0, &path) != aiReturn_SUCCESS || path.length == 0)
				return false;
			return true;
		};

		const auto TryLoadTextureIntoSlot = [&](aiTextureType textureType, MaterialImageType slot, ImageFormat format) -> bool {
			aiString path;
			if (!IsValidTexture(path, textureType))
				return false;

			auto properTexturePath = FileSystem::GetParentPath(importedFilePath) / std::string(path.C_Str());

			const std::filesystem::path canonicalPath = std::filesystem::weakly_canonical(properTexturePath);

			Renderer::EnqueueToRenderCommandQueue([material, properTexturePath, path, slot, format, this, canonicalPath](const Ref<RenderDevice>& device) {
				ImageCreateInfo createInfo{};
				createInfo.Format = format;
				createInfo.ImageType = ImageType::Type2D;
				if (DDS::IsDDS(properTexturePath)) {
					createInfo.ImageUsage = ImageUsage::AsTexture;
					createInfo.GenerateMipmap = MipmapCreateInfo::NoMipmap();
				} else {
					createInfo.ImageUsage = ImageUsage::AsColorAttachment;
					createInfo.GenerateMipmap = MipmapCreateInfo::FromWidthAndHeight();
				}
				createInfo.GenerateSampler = true;
				createInfo.ImGuiUsage = true;

				RenderDeviceResourceHandle textureHandle{};
				if (auto it = m_TextureCache.find(canonicalPath); it != m_TextureCache.end()) {
					textureHandle = it->second;
				} else {
					textureHandle = device->CreateImage(properTexturePath, createInfo, "PBR Image: " + std::string(path.C_Str()));
					m_TextureCache.emplace(canonicalPath, textureHandle);
				}

				material->SetTexture(slot, textureHandle);

				const auto& data = std::any_cast<RenderDevicePBRMaterialData>(material->BuildRenderData(device));
				device->GetScene()->UpdatePBRMaterial(material->GetMaterialDeviceID(), data);
			});

			return true;
		};
		//fucking cancer to support all possible combinations
		switch (material->GetMaterialType()) {
			case MaterialType::PBR: {
				bool loadedBase = TryLoadTextureIntoSlot(aiTextureType_BASE_COLOR, PBRMaterial::ALBEDO_TYPE, ImageFormat::R8G8B8A8_SRGB);
				if (!loadedBase) {
					TryLoadTextureIntoSlot(aiTextureType_DIFFUSE, PBRMaterial::ALBEDO_TYPE, ImageFormat::R8G8B8A8_SRGB);
				}

				bool loadedNormal = TryLoadTextureIntoSlot(aiTextureType_NORMALS, PBRMaterial::NORMALS_TYPE, ImageFormat::R8G8B8A8_UNORM);
				if (!loadedNormal) {
					TryLoadTextureIntoSlot(aiTextureType_NORMAL_CAMERA, PBRMaterial::NORMALS_TYPE, ImageFormat::R8G8B8A8_UNORM);
				}

				TryLoadTextureIntoSlot(aiTextureType_EMISSIVE, PBRMaterial::EMISSIVE_TYPE, ImageFormat::R8G8B8A8_SRGB);

				bool loadedORM = TryLoadTextureIntoSlot(aiTextureType_GLTF_METALLIC_ROUGHNESS, PBRMaterial::ORM_TYPE, ImageFormat::R8G8B8A8_UNORM);
				//fucking cancer to try to support every model
				if (!loadedORM) {
					aiString aoPath;
					bool hasAO = IsValidTexture(aoPath, aiTextureType_AMBIENT_OCCLUSION);

					aiString roughnessPath;
					bool hasRoughness = IsValidTexture(roughnessPath, aiTextureType_DIFFUSE_ROUGHNESS);

					aiString metallicPath;
					bool hasMetallic = IsValidTexture(metallicPath, aiTextureType_METALNESS);

					if (hasAO && hasRoughness && hasMetallic) {
						if (aoPath == roughnessPath && roughnessPath == metallicPath) {
							TryLoadTextureIntoSlot(aiTextureType_DIFFUSE_ROUGHNESS, PBRMaterial::ORM_TYPE, ImageFormat::R8G8B8A8_UNORM);
							break;
						}
					}

					aiString specularPath;
					bool hasSpecular = IsValidTexture(specularPath, aiTextureType_SPECULAR);

					if (hasSpecular) {
						bool packedSpecularTexture = false;
						
						if (hasAO && specularPath == aoPath)
							packedSpecularTexture = true;
						if (hasRoughness && specularPath == roughnessPath)
							packedSpecularTexture = true;
						if (hasMetallic && specularPath == metallicPath)
							packedSpecularTexture = true;

						// Amazon Lumberyard Bistro stores AO, roughness and metallic
						// packed into the specular texture
						if (packedSpecularTexture || (!hasAO && !hasRoughness && !hasMetallic)) {
							TryLoadTextureIntoSlot(aiTextureType_SPECULAR, PBRMaterial::ORM_TYPE, ImageFormat::R8G8B8A8_UNORM);
							break;
						}
					}

					TryLoadTextureIntoSlot(aiTextureType_AMBIENT_OCCLUSION, PBRMaterial::AO_TYPE, ImageFormat::R8G8B8A8_UNORM);
					TryLoadTextureIntoSlot(aiTextureType_DIFFUSE_ROUGHNESS, PBRMaterial::ROUGHNESS_TYPE, ImageFormat::R8G8B8A8_UNORM);
					TryLoadTextureIntoSlot(aiTextureType_METALNESS, PBRMaterial::METALLIC_TYPE, ImageFormat::R8G8B8A8_UNORM);
				}
				break;
			}
			default:
				LUCY_ASSERT(false, "Material type not yet implemented!");
				break;
		}
	}
}