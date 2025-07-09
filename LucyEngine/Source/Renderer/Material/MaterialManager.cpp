#include "lypch.h"
#include "MaterialManager.h"
#include "PBRMaterial.h"

#include "Core/FileSystem.h"

#include "assimp/material.h"

namespace Lucy {
	
	MaterialManager::MaterialManager(const std::unordered_map<std::string, Ref<Shader>>& shaders)
		: m_Shaders(shaders) {
	}

	MaterialID MaterialManager::CreateMaterialByPath(MaterialType materialType, aiMaterial* aiMaterial, const std::string& importedFilePath) {
		switch (materialType) {
			case MaterialType::PBR:
				return CreatePBRMaterial(aiMaterial, importedFilePath);
			default:
				LUCY_ASSERT(false, "Other material types aren't implemented yet!");
				break;
		}
		return 0;
	}

	void MaterialManager::RTDestroyMaterial(MaterialID materialID) {
		s_MaterialIDProvider.ReturnID(materialID);
		m_Materials.at(materialID)->RTDestroyResource();
	}

	void MaterialManager::RTDestroyMaterials(const std::vector<MaterialID>& materialIDs) {
		for (MaterialID materialID : materialIDs)
			RTDestroyMaterial(materialID);
	}

	void MaterialManager::DestroyAll() {
		for (const Ref<Material>& material : m_Materials | std::views::values)
			material->RTDestroyResource();
	}

	//TODO: Material update system (currently, its updating every frame -> bad for performance)
	void MaterialManager::UpdateMaterialsIfNecessary() {
		LUCY_PROFILE_NEW_EVENT("MaterialManager::UpdateMaterialsIfNecessary");
		for (Ref<Material> material : m_Materials | std::views::values)
			material->Update();
	}

	MaterialID MaterialManager::CreatePBRMaterial(aiMaterial* aiMaterial, const std::string& importedFilePath) {
		aiColor3D diffuse;
		float shininess = 0.0f, metallic = 0.0f, roughness = 0.0f, aoContribution = 1.0f;

		aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
		aiMaterial->Get(AI_MATKEY_SHININESS, shininess);
		aiMaterial->Get(AI_MATKEY_REFLECTIVITY, metallic);

		if (metallic < 0)
			metallic = 0.0f;

		if (shininess < 0)
			shininess = 0.0f;

		roughness = 1.0f - glm::sqrt(shininess / 100.0f);

		PBRMaterialData materialData(glm::vec3(diffuse.r, diffuse.g, diffuse.b), metallic, roughness, aoContribution);
		MaterialID materialID = s_MaterialIDProvider.RequestID();

		auto LoadPBRTexture = [](auto aiMaterial, aiTextureType textureType, const std::string& importedFilePath, const Ref<PBRMaterial>& outMaterial) {
			aiString path;
			if (aiMaterial->GetTexture(textureType, 0, &path) == aiReturn_SUCCESS) {
				auto properTexturePath = FileSystem::GetParentPath(importedFilePath) / std::string(path.data);

				Renderer::EnqueueToRenderCommandQueue([outMaterial, path, properTexturePath](const Ref<RenderDevice>& device) {
					ImageCreateInfo createInfo;
					createInfo.Format = ImageFormat::R8G8B8A8_UNORM;
					createInfo.ImageType = ImageType::Type2D;
					createInfo.ImageUsage = ImageUsage::AsColorAttachment;
					createInfo.Parameter.Mag = ImageFilterMode::LINEAR;
					createInfo.Parameter.Min = ImageFilterMode::LINEAR;
					createInfo.Parameter.U = ImageAddressMode::REPEAT;
					createInfo.Parameter.V = ImageAddressMode::REPEAT;
					createInfo.Parameter.W = ImageAddressMode::REPEAT;
					createInfo.GenerateMipmap = true;
					createInfo.ImGuiUsage = true;
					createInfo.GenerateSampler = true;

					RenderResourceHandle texture2DHandle = device->CreateImage(properTexturePath, createInfo);
					outMaterial->AddTexture(texture2DHandle);
				});
			} else {
				if (path.data)
					LUCY_WARN(std::format("Texture id: {0} could not be loaded: {1}", (uint32_t)textureType, path.data));
			}
		};

		m_Materials.try_emplace(materialID, Memory::CreateRef<PBRMaterial>(materialID, m_Shaders.at("LucyPBR"), materialData));
		const Ref<PBRMaterial>& pbrMaterial = m_Materials[materialID]->As<PBRMaterial>();

		LoadPBRTexture(aiMaterial, aiTextureType_DIFFUSE, importedFilePath, pbrMaterial);
		LoadPBRTexture(aiMaterial, aiTextureType_HEIGHT, importedFilePath, pbrMaterial);
		LoadPBRTexture(aiMaterial, aiTextureType_SHININESS, importedFilePath, pbrMaterial);
		LoadPBRTexture(aiMaterial, aiTextureType_SPECULAR, importedFilePath, pbrMaterial);
		LoadPBRTexture(aiMaterial, aiTextureType_AMBIENT_OCCLUSION, importedFilePath, pbrMaterial);

		return materialID;
	}
}