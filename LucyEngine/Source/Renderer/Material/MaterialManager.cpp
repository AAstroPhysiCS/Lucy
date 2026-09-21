#include "lypch.h"

#include <numeric>

#include "MaterialManager.h"
#include "PBRMaterial.h"

#include "Renderer/Device/RenderDeviceScene.h"

#include "Renderer/Pipeline/GraphicsPipeline.h"
#include "Renderer/Image/Image.h"
#include "Renderer/Image/DDSHelper.h"

#include "Core/FileSystem.h"
#include "Scene/SceneImporter.h"

namespace Lucy {

	MaterialManager::MaterialManager(const Unique<PipelineManager>& pipelineManager)
		: m_PipelineManager(pipelineManager) {
	}

	RenderDeviceObjectHandle MaterialManager::CreateMaterialByPath(MaterialType materialType, const ImportedMaterial& importedMaterial, const std::string& importedFilePath) {
		switch (materialType) {
			case MaterialType::PBR:
				return CreatePBRMaterial(importedMaterial, importedFilePath);
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

	RenderDeviceObjectHandle MaterialManager::CreatePBRMaterial(const ImportedMaterial& importedMaterial, const std::string& importedFilePath) {
		RenderDevicePBRMaterialData materialGPUData{};
		materialGPUData.BaseColor = importedMaterial.BaseColor;
		materialGPUData.ORME = glm::vec4{ importedMaterial.AO, importedMaterial.Roughness, importedMaterial.Metallic, importedMaterial.EmissiveStrength };
		materialGPUData.NormalStrength = importedMaterial.NormalStrength;
		materialGPUData.Specular = glm::vec4{ importedMaterial.SpecularColor, importedMaterial.SpecularFactor };
		materialGPUData.Clearcoat = glm::vec2{ importedMaterial.ClearcoatFactor, importedMaterial.ClearcoatRoughness };
		if (importedMaterial.DoubleSided)
			materialGPUData.Flags |= static_cast<uint32_t>(RenderDevicePBRMaterialFlags::DoubleSided);

		RenderDeviceObjectHandle materialDeviceHandle = Renderer::GetRenderDevice()->GetScene()->RegisterPBRMaterial(materialGPUData);

		MaterialCreateInfo createInfo;
		createInfo.MaterialType = MaterialType::PBR;
		createInfo.MaterialDeviceID = materialDeviceHandle;

		Ref<PBRMaterial> material = Memory::CreateRef<PBRMaterial>(createInfo, materialGPUData);
		LoadMaterialTextures(importedMaterial, importedFilePath, material);
		m_Materials.try_emplace(materialDeviceHandle, material);

		return materialDeviceHandle;
	}

	void MaterialManager::LoadMaterialTextures(const ImportedMaterial& importedMaterial, const std::string& importedFilePath, const Ref<Material>& material) {
		const auto TryLoadTextureIntoSlot = [&](const std::string& path, MaterialImageType slot, ImageFormat format) -> bool {
			if (path.empty())
				return false;

			auto properTexturePath = FileSystem::GetParentPath(importedFilePath) / path;
			auto canonicalPath = FileSystem::WeaklyCanonical(properTexturePath);

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
					textureHandle = device->CreateImage(properTexturePath, createInfo, "PBR Image: " + path);
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
				bool loadedBase = TryLoadTextureIntoSlot(importedMaterial.Textures.Albedo, PBRMaterial::ALBEDO_TYPE, ImageFormat::R8G8B8A8_SRGB);
				if (!loadedBase) {
					TryLoadTextureIntoSlot(importedMaterial.Textures.Diffuse, PBRMaterial::ALBEDO_TYPE, ImageFormat::R8G8B8A8_SRGB);
				}

				bool loadedNormal = TryLoadTextureIntoSlot(importedMaterial.Textures.Normal, PBRMaterial::NORMALS_TYPE, ImageFormat::R8G8B8A8_UNORM);
				if (!loadedNormal) {
					TryLoadTextureIntoSlot(importedMaterial.Textures.NormalCamera, PBRMaterial::NORMALS_TYPE, ImageFormat::R8G8B8A8_UNORM);
				}

				TryLoadTextureIntoSlot(importedMaterial.Textures.Emissive, PBRMaterial::EMISSIVE_TYPE, ImageFormat::R8G8B8A8_SRGB);

				bool loadedORM = TryLoadTextureIntoSlot(importedMaterial.Textures.ORM, PBRMaterial::ORM_TYPE, ImageFormat::R8G8B8A8_UNORM);
				if (!loadedORM) {
					//fucking cancer to try to support every model

					const std::string& aoPath = importedMaterial.Textures.AO;
					const std::string& roughnessPath = importedMaterial.Textures.Roughness;
					const std::string& metallicPath = importedMaterial.Textures.Metallic;
					const std::string& specularPath = importedMaterial.Textures.Specular;

					bool hasAO = !aoPath.empty();
					bool hasRoughness = !roughnessPath.empty();
					bool hasMetallic = !metallicPath.empty();
					bool hasSpecular = !specularPath.empty();

					if (hasAO && hasRoughness && hasMetallic) {
						if (aoPath == roughnessPath && roughnessPath == metallicPath) {
							TryLoadTextureIntoSlot(roughnessPath, PBRMaterial::ORM_TYPE, ImageFormat::R8G8B8A8_UNORM);
							break;
						}
					}

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
							TryLoadTextureIntoSlot(specularPath, PBRMaterial::ORM_TYPE, ImageFormat::R8G8B8A8_UNORM);
							break;
						}
					}

					TryLoadTextureIntoSlot(aoPath, PBRMaterial::AO_TYPE, ImageFormat::R8G8B8A8_UNORM);
					TryLoadTextureIntoSlot(roughnessPath, PBRMaterial::ROUGHNESS_TYPE, ImageFormat::R8G8B8A8_UNORM);
					TryLoadTextureIntoSlot(metallicPath, PBRMaterial::METALLIC_TYPE, ImageFormat::R8G8B8A8_UNORM);
				}
				break;
			}
			default:
				LUCY_ASSERT(false, "Material type not yet implemented!");
				break;
		}
	}
}