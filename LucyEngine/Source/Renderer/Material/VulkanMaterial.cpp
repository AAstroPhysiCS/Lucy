#include "lypch.h"
#include "VulkanMaterial.h"

#include "Renderer/Renderer.h"
#include "Renderer/Context/Pipeline.h"
#include "Renderer/Memory/Buffer/Vulkan/VulkanUniformBuffer.h"
#include "Renderer/Memory/Buffer/Vulkan/VulkanSharedStorageBuffer.h"

namespace Lucy {

	VulkanMaterial::VulkanMaterial(Ref<Shader> shader, aiMaterial* aiMaterial, const char* submeshName, const std::string& importedFilePath)
		: Material(shader, aiMaterial, submeshName, importedFilePath) {
		LoadTexture(aiMaterial, ALBEDO_TYPE, importedFilePath);
		LoadTexture(aiMaterial, NORMALS_TYPE, importedFilePath);
		LoadTexture(aiMaterial, METALLIC_TYPE, importedFilePath);
		LoadTexture(aiMaterial, ROUGHNESS_TYPE, importedFilePath);
		LoadTexture(aiMaterial, AO_TYPE, importedFilePath);
	}

	void VulkanMaterial::Update(Ref<Pipeline> pipeline) {
		auto& uniformImageBuffer = pipeline->GetUniformBuffers<VulkanUniformImageBuffer>("u_Textures");
		auto& ssboMaterialAttributes = pipeline->GetSharedStorageBuffers<VulkanSharedStorageBuffer>("LucyMaterialAttributes");

		if (HasImage(Material::ALBEDO_TYPE)) {
			uint32_t pos = uniformImageBuffer->BindImage(GetImage(Material::ALBEDO_TYPE));
			m_MaterialShaderData.AlbedoSlot = pos;
		}
		else if (HasImage(Material::NORMALS_TYPE)) {
			uint32_t pos = uniformImageBuffer->BindImage(GetImage(Material::NORMALS_TYPE));
			m_MaterialShaderData.NormalSlot = pos;
		}
		else if (HasImage(Material::METALLIC_TYPE)) {
			uint32_t pos = uniformImageBuffer->BindImage(GetImage(Material::METALLIC_TYPE));
			m_MaterialShaderData.MetallicSlot = pos;
		}
		else if (HasImage(Material::ROUGHNESS_TYPE)) {
			uint32_t pos = uniformImageBuffer->BindImage(GetImage(Material::ROUGHNESS_TYPE));
			m_MaterialShaderData.RoughnessSlot = pos;
		}
		else if (HasImage(Material::AO_TYPE)) {
			uint32_t pos = uniformImageBuffer->BindImage(GetImage(Material::AO_TYPE));
			m_MaterialShaderData.AOSlot = pos;
		}

		//TODO: Change diffuse color to vec4
		m_MaterialShaderData.BaseDiffuseColor = glm::vec4(m_MaterialData.Diffuse, 1.0f);
		m_MaterialShaderData.Reflectivity = m_MaterialData.Reflectivity;
		m_MaterialShaderData.Roughness = m_MaterialData.Roughness;
		m_MaterialShaderData.Shininess = m_MaterialData.Shininess;

		ssboMaterialAttributes->Append((uint8_t*)&m_MaterialShaderData, sizeof(m_MaterialShaderData));
	}

	void VulkanMaterial::LoadTexture(aiMaterial* aiMaterial, MaterialImageType type, const std::string& importedFilePath) {
		aiString path;
		if (aiMaterial->GetTexture((aiTextureType)type.Type, 0, &path) == aiReturn_SUCCESS) {
			std::string properTexturePath = FileSystem::GetParentPath(importedFilePath) + "/" + std::string(path.data);

			ImageCreateInfo createInfo;
			createInfo.Format = VK_FORMAT_R8G8B8A8_UNORM;
			createInfo.Target = ImageTarget::Color;
			createInfo.ImageType = ImageType::Type2D;
			createInfo.Parameter.Mag = VK_FILTER_LINEAR;
			createInfo.Parameter.Min = VK_FILTER_LINEAR;
			createInfo.Parameter.U = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.Parameter.V = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.Parameter.W = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.GenerateMipmap = true;

			Ref<VulkanRHIImageDesc> imageDesc = Memory::CreateRef<VulkanRHIImageDesc>();
			imageDesc->ImGuiUsage = true;
			imageDesc->GenerateSampler = true;

			createInfo.InternalInfo = imageDesc;

			Ref<Image2D> texture2D = Image2D::Create(properTexturePath, createInfo);
			m_Textures.push_back(texture2D);
		} else {
			LUCY_WARN(fmt::format("Texture {0} could not be loaded: {1}", type.Name, path.data));
		}
	}
}