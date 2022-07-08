#include "lypch.h"
#include "VulkanMaterial.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	VulkanMaterial::VulkanMaterial(Ref<Shader> shader, aiMaterial* aiMaterial, const char* submeshName, std::string& importedFilePath)
		: Material(shader, aiMaterial, submeshName, importedFilePath) {
		LoadTexture(aiMaterial, 1, ALBEDO_TYPE, importedFilePath);
		LoadTexture(aiMaterial, 2, NORMALS_TYPE, importedFilePath);
		LoadTexture(aiMaterial, 3, METALLIC_TYPE, importedFilePath);
		LoadTexture(aiMaterial, 4, ROUGHNESS_TYPE, importedFilePath);
		LoadTexture(aiMaterial, 5, AO_TYPE, importedFilePath);
	}

	void VulkanMaterial::Update(Ref<Pipeline> pipeline) {
		/*
		auto& uniformBuffer = pipeline->GetUniformBuffers<VulkanUniformBuffer>("TextureSlots");

		if (HasTexture(Material::ALBEDO_TYPE)) {
			uniformBuffer->SetData((void*)&Material::ALBEDO_TYPE.Slot, sizeof(int32_t), 0);
		} else if (HasTexture(Material::NORMALS_TYPE)) {
			uniformBuffer->SetData((void*)&Material::NORMALS_TYPE.Slot, sizeof(int32_t), sizeof(int32_t));
		} else if (HasTexture(Material::METALLIC_TYPE)) {
			uniformBuffer->SetData((void*)&Material::METALLIC_TYPE.Slot, sizeof(int32_t), sizeof(int32_t) * 2);
		} else if (HasTexture(Material::ROUGHNESS_TYPE)) {
			uniformBuffer->SetData((void*)&Material::ROUGHNESS_TYPE.Slot, sizeof(int32_t), sizeof(int32_t) * 3);
		} else if (HasTexture(Material::AO_TYPE)) {
			uniformBuffer->SetData((void*)&Material::AO_TYPE.Slot, sizeof(int32_t), sizeof(int32_t) * 4);
		}
		uniformBuffer->Update();
		*/
		
	}

	void VulkanMaterial::LoadTexture(aiMaterial* aiMaterial, TextureSlot slot, TextureType type, std::string& importedFilePath) {
		aiString path;
		if (aiMaterial->GetTexture((aiTextureType)type.Type, 0, &path) == aiReturn_SUCCESS) {
			std::string properTexturePath = FileSystem::GetParentPath(importedFilePath) + "/" + std::string(path.data);

			ImageCreateInfo createInfo;
			createInfo.Format = VK_FORMAT_R8G8B8A8_UNORM;
			createInfo.ImageType = ImageType::Type2D;
			createInfo.Parameter.Mag = VK_FILTER_LINEAR;
			createInfo.Parameter.Min = VK_FILTER_LINEAR;
			createInfo.Parameter.U = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.Parameter.V = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.Parameter.W = VK_SAMPLER_ADDRESS_MODE_REPEAT;

			Ref<VulkanRHIImageDesc> imageDesc = Memory::CreateRef<VulkanRHIImageDesc>();
			imageDesc->GenerateSampler = true;

			createInfo.InternalInfo = imageDesc;

			Ref<Image2D> texture2D = Image2D::Create(properTexturePath, createInfo);
			m_Textures.push_back(texture2D);
		} else {
			LUCY_WARN(fmt::format("Texture {0} could not be loaded: {1}", type.Name, path.data));
		}
	}
}