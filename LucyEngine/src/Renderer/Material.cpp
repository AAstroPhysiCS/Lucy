#include "Material.h"

#include "../Core/FileSystem.h"
#include "Texture/Texture.h"
#include "glm/glm.hpp"

#include "glad/glad.h"

namespace Lucy {

	Material::Material(RefLucy<Shader> shader, aiMaterial* aiMaterial, std::string& importedFilePath)
		: m_Shader(shader) {
		aiColor3D diffuse;
		aiString name;
		float shininess, reflectivity, roughness;

		aiMaterial->Get(AI_MATKEY_NAME, name);
		aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
		aiMaterial->Get(AI_MATKEY_SHININESS, shininess);
		aiMaterial->Get(AI_MATKEY_REFLECTIVITY, reflectivity);

		roughness = 1.0f - glm::sqrt(shininess / 100.0f);

		m_MaterialData = MaterialData(*(glm::vec3*)&diffuse, std::string(name.data), shininess, reflectivity, roughness);

		LoadTexture(aiMaterial, 1, { aiTextureType_DIFFUSE, "Diffuse" }, importedFilePath);
		LoadTexture(aiMaterial, 2, { aiTextureType_HEIGHT, "Height" }, importedFilePath);
		LoadTexture(aiMaterial, 3, { aiTextureType_SHININESS, "Shininess" }, importedFilePath);
		LoadTexture(aiMaterial, 4, { aiTextureType_SPECULAR, "Specular" }, importedFilePath);
		LoadTexture(aiMaterial, 5, { aiTextureType_AMBIENT_OCCLUSION, "AO" }, importedFilePath);
	}

	void Material::Bind() {
		for (uint32_t i = 0; i < m_Textures.size(); i++) {
			glBindTextureUnit(m_Textures[i]->GetSlot(), m_Textures[i]->GetID());
		}
		m_Shader->Bind();
	}

	void Material::Unbind() {
		m_Shader->Unbind();
		for (uint32_t i = 0; i < m_Textures.size(); i++) {
			glBindTextureUnit(m_Textures[i]->GetSlot(), 0);
		}
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
		}
		else {
			LUCY_WARN(fmt::format("Texture {0} could not be loaded: {1}", type.Name, path.data));
		}
	}
}