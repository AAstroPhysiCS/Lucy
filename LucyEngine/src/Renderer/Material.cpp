#include "lypch.h"

#include "Material.h"
#include "Shader/Shader.h"

#include "../Core/FileSystem.h"
#include "Texture/Texture.h"

#include "glm/glm.hpp"
#include "glad/glad.h"

#include "Buffer/UniformBuffer.h"
#include "Renderer/Renderer.h"

namespace Lucy {

	Material::Material(RefLucy<Shader> shader, aiMaterial* aiMaterial, const char* submeshName, std::string& importedFilePath)
		: m_Shader(shader) {
		aiColor3D diffuse;
		float shininess, reflectivity, roughness;

		aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
		aiMaterial->Get(AI_MATKEY_SHININESS, shininess);
		aiMaterial->Get(AI_MATKEY_REFLECTIVITY, reflectivity);

		roughness = 1.0f - glm::sqrt(shininess / 100.0f);

		m_MaterialData = MaterialData(*(glm::vec3*)&diffuse, std::string(submeshName), shininess, reflectivity, roughness);

		LoadTexture(aiMaterial, 1, ALBEDO_TYPE, importedFilePath);
		LoadTexture(aiMaterial, 2, NORMALS_TYPE, importedFilePath);
		LoadTexture(aiMaterial, 3, METALLIC_TYPE, importedFilePath);
		LoadTexture(aiMaterial, 4, ROUGHNESS_TYPE, importedFilePath);
		LoadTexture(aiMaterial, 5, AO_TYPE, importedFilePath);
	}

	void Material::Bind() {
		m_Shader->Bind();
		for (uint32_t i = 0; i < m_Textures.size(); i++) {
			uint32_t slot = m_Textures[i]->GetSlot();
			glBindTextureUnit(slot, m_Textures[i]->GetID());
		}
		
		RefLucy<RendererAPI>& currentContext = Renderer::GetCurrentRenderer();

		//TODO: Kinda bad/ugly code and its slow somehow although i dont do anything wrong here...!
		if (HasTexture(Material::ALBEDO_TYPE)) {
			int32_t albedoSlot = GetTexture(Material::ALBEDO_TYPE)->GetSlot();
			currentContext->m_TextureSlotsUniformBuffer->SetData((void*)&albedoSlot, sizeof(int32_t), 0);
		} else if (HasTexture(Material::NORMALS_TYPE)) {
			int32_t normalsSlot = GetTexture(Material::NORMALS_TYPE)->GetSlot();
			currentContext->m_TextureSlotsUniformBuffer->SetData((void*)&normalsSlot, sizeof(int32_t), sizeof(int32_t));
		} else if (HasTexture(Material::METALLIC_TYPE)) {
			int32_t metallicSlot = GetTexture(Material::METALLIC_TYPE)->GetSlot();
			currentContext->m_TextureSlotsUniformBuffer->SetData((void*)&metallicSlot, sizeof(int32_t), sizeof(int32_t) * 2);
		} else if (HasTexture(Material::ROUGHNESS_TYPE)) {
			int32_t roughnessSlot = GetTexture(Material::ROUGHNESS_TYPE)->GetSlot();
			currentContext->m_TextureSlotsUniformBuffer->SetData((void*)&roughnessSlot, sizeof(int32_t), sizeof(int32_t) * 3);
		} else if (HasTexture(Material::AO_TYPE)) {
			int32_t aoSlot = GetTexture(Material::AO_TYPE)->GetSlot();
			currentContext->m_TextureSlotsUniformBuffer->SetData((void*)&aoSlot, sizeof(int32_t), sizeof(int32_t) * 4);
		}
	}

	void Material::Unbind() {
		int32_t clearValue = -1;
		RefLucy<RendererAPI>& currentContext = Renderer::GetCurrentRenderer();
		currentContext->m_TextureSlotsUniformBuffer->SetData((void*)&clearValue, sizeof(int32_t) * 5, 0);
		m_Shader->Unbind();
		for (uint32_t i = 0; i < m_Textures.size(); i++) {
			glBindTextureUnit(m_Textures[i]->GetSlot(), 0);
		}
	}

	bool Material::HasTexture(TextureType type) {
		return !m_Textures.empty() && m_Textures.size() > type.Index;
	}

	void Material::LoadTexture(aiMaterial* aiMaterial, TextureSlot slot, TextureType type, std::string& importedFilePath) {
		aiString path;
		if (aiMaterial->GetTexture((aiTextureType)type.Type, 0, &path) == aiReturn_SUCCESS) {
			std::string properTexturePath = FileSystem::GetParentPath(importedFilePath) + "/" + std::string(path.data);

			TextureSpecification specs;
			specs.Path = properTexturePath.c_str();
			specs.GenerateMipmap = true;
			specs.PixelType = PixelType::UnsignedByte;
			specs.Parameter = { 0, GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR };
			specs.Slot = slot;

			RefLucy<Texture2D> texture2D = Texture2D::Create(specs);
			m_Textures.push_back(texture2D);
		} else {
			LUCY_WARN(fmt::format("Texture {0} could not be loaded: {1}", type.Name, path.data));
		}
	}
}