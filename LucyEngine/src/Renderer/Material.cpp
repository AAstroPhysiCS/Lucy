#include "lypch.h"

#include "Material.h"

#include "../Core/FileSystem.h"
#include "Texture/Texture.h"
#include "glm/glm.hpp"

#include "glad/glad.h"

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

			//Kinda bad code
			if (slot == Material::ALBEDO_TYPE.Slot)
				m_Shader->SetInt("u_AlbedoTextureSlot", slot);
			if (slot == Material::NORMALS_TYPE.Slot)
				m_Shader->SetInt("u_NormalsTextureSlot", slot);
			if (slot == Material::METALLIC_TYPE.Slot)
				m_Shader->SetInt("u_MetallicTextureSlot", slot);
			if (slot == Material::ROUGHNESS_TYPE.Slot)
				m_Shader->SetInt("u_RoughnessTextureSlot", slot);
			if (slot == Material::AO_TYPE.Slot)
				m_Shader->SetInt("u_AOTextureSlot", slot);
		}
	}

	void Material::Unbind() {
		m_Shader->SetInt("u_AlbedoTextureSlot", -1);
		m_Shader->SetInt("u_NormalsTextureSlot", -1);
		m_Shader->SetInt("u_MetallicTextureSlot", -1);
		m_Shader->SetInt("u_RoughnessTextureSlot", -1);
		m_Shader->SetInt("u_AOTextureSlot", -1);
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