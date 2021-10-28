#include "Material.h"

#include "../Core/FileSystem.h"
#include "Texture/Texture.h"
#include "glm/glm.hpp"

namespace Lucy {
	
	Material::Material(aiMaterial* aiMaterial, std::string& importedFilePath)
	{
		aiColor3D diffuse;
		aiString name;
		float shininess, reflectivity, roughness;

		aiMaterial->Get(AI_MATKEY_NAME, name);
		aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
		aiMaterial->Get(AI_MATKEY_SHININESS, shininess);
		aiMaterial->Get(AI_MATKEY_REFLECTIVITY, reflectivity);

		roughness = 1.0f - glm::sqrt(shininess / 100.0f);
		
		m_MaterialData = MaterialData( *(glm::vec3*)&diffuse, std::string(name.data), shininess, reflectivity, roughness );

		LoadTexture(aiMaterial, { aiTextureType_DIFFUSE, "Diffuse"}, importedFilePath);
		LoadTexture(aiMaterial, { aiTextureType_HEIGHT, "Height" }, importedFilePath);
		LoadTexture(aiMaterial, { aiTextureType_SHININESS, "Shininess" }, importedFilePath);
		LoadTexture(aiMaterial, { aiTextureType_SPECULAR, "Specular" }, importedFilePath);
		LoadTexture(aiMaterial, { aiTextureType_AMBIENT_OCCLUSION, "AO" }, importedFilePath);
	}

	void Material::LoadTexture(aiMaterial* aiMaterial, TextureType type, std::string& importedFilePath)
	{
		aiString path;
		if (aiMaterial->GetTexture((aiTextureType) type.Type, 0, &path) == aiReturn_SUCCESS) {
			std::string properTexturePath = FileSystem::GetParentPath(importedFilePath) + "/" + std::string(path.data);

			TextureSpecification specs;
			specs.Path = properTexturePath.c_str();
			//TODO: Do this
			
			RefLucy<Texture2D> texture2D = Texture2D::Create(specs);
			m_Textures.push_back(texture2D);
		} else {
			LUCY_WARN(fmt::format("Texture {0} could not be loaded: {1}", type.Name, path.data));
		}
	}
}