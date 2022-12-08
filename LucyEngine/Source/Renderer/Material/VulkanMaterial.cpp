#include "lypch.h"
#include "VulkanMaterial.h"

#include "Core/FileSystem.h"

#include "Renderer/Renderer.h"
#include "Renderer/Context/GraphicsPipeline.h"
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

	void VulkanMaterial::Update(Ref<GraphicsPipeline> pipeline) {
		LUCY_PROFILE_NEW_EVENT("VulkanMaterial::Update");

		auto uniformImageBuffer = pipeline->GetUniformBuffers<VulkanUniformImageBuffer>("u_Textures");
		auto ssboMaterialAttributes = pipeline->GetSharedStorageBuffers<VulkanSharedStorageBuffer>("LucyMaterialAttributes");

		if (HasImage(Material::ALBEDO_TYPE)) {
			uint32_t pos = uniformImageBuffer->BindImage(GetImage(Material::ALBEDO_TYPE));
			m_MaterialShaderData.AlbedoSlot = (float)pos;
		}
		else if (HasImage(Material::NORMALS_TYPE)) {
			uint32_t pos = uniformImageBuffer->BindImage(GetImage(Material::NORMALS_TYPE));
			m_MaterialShaderData.NormalSlot = (float)pos;
		}
		else if (HasImage(Material::METALLIC_TYPE)) {
			uint32_t pos = uniformImageBuffer->BindImage(GetImage(Material::METALLIC_TYPE));
			m_MaterialShaderData.MetallicSlot = (float)pos;
		}
		else if (HasImage(Material::ROUGHNESS_TYPE)) {
			uint32_t pos = uniformImageBuffer->BindImage(GetImage(Material::ROUGHNESS_TYPE));
			m_MaterialShaderData.RoughnessSlot = (float)pos;
		}
		else if (HasImage(Material::AO_TYPE)) {
			uint32_t pos = uniformImageBuffer->BindImage(GetImage(Material::AO_TYPE));
			m_MaterialShaderData.AOSlot = (float)pos;
		}

		//TODO: Change diffuse color to vec4
		m_MaterialShaderData.BaseDiffuseColor = glm::vec4(m_MaterialData.Diffuse, 1.0f);
		m_MaterialShaderData.BaseMetallicValue = m_MaterialData.Metallic;
		m_MaterialShaderData.BaseRoughnessValue = m_MaterialData.Roughness;
		m_MaterialShaderData.BaseAOValue = m_MaterialData.AOContribution;

		ssboMaterialAttributes->Append((uint8_t*)&m_MaterialShaderData, sizeof(m_MaterialShaderData));
	}

	void VulkanMaterial::LoadTexture(aiMaterial* aiMaterial, const MaterialImageType& type, const std::string& importedFilePath) {
		aiString path;
		if (aiMaterial->GetTexture((aiTextureType)type.Type, 0, &path) == aiReturn_SUCCESS) {
			std::string properTexturePath = FileSystem::GetParentPath(importedFilePath) + "/" + std::string(path.data);

			ImageCreateInfo createInfo;
			createInfo.Format = ImageFormat::R8G8B8A8_UNORM;
			createInfo.ImageType = ImageType::Type2DColor;
			createInfo.Parameter.Mag = ImageFilterMode::LINEAR;
			createInfo.Parameter.Min = ImageFilterMode::LINEAR;
			createInfo.Parameter.U = ImageAddressMode::REPEAT;
			createInfo.Parameter.V = ImageAddressMode::REPEAT;
			createInfo.Parameter.W = ImageAddressMode::REPEAT;
			createInfo.GenerateMipmap = true;
			createInfo.ImGuiUsage = true;
			createInfo.GenerateSampler = true;

			Ref<Image> texture2D = Image::Create(properTexturePath, createInfo);
			m_Textures.push_back(texture2D);
		} else {
			LUCY_WARN(fmt::format("Texture {0} could not be loaded: {1}", type.Name, path.data));
		}
	}
}