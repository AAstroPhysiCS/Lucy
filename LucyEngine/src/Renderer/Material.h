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
		Material(RefLucy<Shader> shader, aiMaterial* aiMaterial, const char* submeshName, std::string& importedFilePath);

		void Bind();
		void Unbind();

		inline RefLucy<Shader> GetShader() const { return m_Shader; }
		inline std::string GetName() const { return m_MaterialData.Name; }

		bool HasTexture(TextureType type);
		inline RefLucy<Texture2D> GetTexture(TextureType type) const { return m_Textures[type.Index]; }

		inline static const TextureType ALBEDO_TYPE = { aiTextureType_DIFFUSE, "Albedo", 1, 0 };
		inline static const TextureType NORMALS_TYPE = { aiTextureType_HEIGHT, "Height", 2, 1 };
		inline static const TextureType METALLIC_TYPE = { aiTextureType_SHININESS, "Metallic", 3, 2 };
		inline static const TextureType ROUGHNESS_TYPE = { aiTextureType_SPECULAR, "Roughness", 4, 3 };
		inline static const TextureType AO_TYPE = { aiTextureType_AMBIENT_OCCLUSION, "Ambient Occlusion", 5, 4 };
	private:
		void LoadTexture(aiMaterial* aiMaterial, TextureSlot slot, TextureType type, std::string& importedFilePath);

		MaterialData m_MaterialData;
		RefLucy<Shader> m_Shader;
		std::vector<RefLucy<Texture2D>> m_Textures;
	};
}


