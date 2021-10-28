#pragma once

#include "../Core/Base.h"
#include "Texture/Texture.h"

#include "glm/glm.hpp"
#include "assimp/scene.h"

namespace Lucy {

	struct MaterialData {
		glm::vec3 Diffuse;
		std::string Name;
		float Shininess, Reflectivity, Roughness;

		MaterialData(glm::vec3& diffuse, std::string& name, float shininess, float reflectivity, float roughness) 
			: Diffuse(diffuse), Name(name), Shininess(shininess), Reflectivity(reflectivity), Roughness(roughness)
		{
		}
		MaterialData() = default;
	};

	class Material
	{
	public:
		Material(aiMaterial* aiMaterial, std::string& importedFilePath);

	private:
		void LoadTexture(aiMaterial* aiMaterial, TextureType type, std::string& importedFilePath);

		MaterialData m_MaterialData;
		std::vector<RefLucy<Texture2D>> m_Textures;
	};
}


