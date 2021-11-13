#pragma once

#include "../Core/Base.h"
#include "Texture/Texture.h"
#include "Shader/Shader.h"

#include "glm/glm.hpp"
#include "assimp/scene.h"

namespace Lucy {

	struct MaterialData {
		glm::vec3 Diffuse = glm::vec3();
		std::string Name = "Unknown Material Data";
		float Shininess = 0.0f, Reflectivity = 0.0f, Roughness = 0.0f;

		MaterialData(glm::vec3& diffuse, std::string& name, float shininess, float reflectivity, float roughness)
			: Diffuse(diffuse), Name(name), Shininess(shininess), Reflectivity(reflectivity), Roughness(roughness) {
		}
		MaterialData() = default;
	};

	class Material {
	public:
		Material() = default;
		Material(RefLucy<Shader> shader, aiMaterial* aiMaterial, std::string& importedFilePath);

		void Bind();
		void Unbind();

		inline RefLucy<Shader> GetShader() const { return m_Shader; }

	private:
		void LoadTexture(aiMaterial* aiMaterial, TextureSlot slot, TextureType type, std::string& importedFilePath);

		MaterialData m_MaterialData;
		RefLucy<Shader> m_Shader;
		std::vector<RefLucy<Texture2D>> m_Textures;
	};
}


