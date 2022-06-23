#pragma once

#include "../Core/Base.h"
#include "../Core/FileSystem.h"

#include "glm/glm.hpp"
#include "assimp/scene.h"

#include "Shader/Shader.h"
#include "Image/Image.h"

#include "glm/glm.hpp"
#include "glad/glad.h"

namespace Lucy {

	class Pipeline;

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
		static Ref<Material> Create(Ref<Shader> shader, aiMaterial* aiMaterial, const char* submeshName, std::string& importedFilePath);

		Material(Ref<Shader> shader, aiMaterial* aiMaterial, const char* submeshName, std::string& importedFilePath);
		virtual ~Material() = default;

		virtual void Bind(Ref<Pipeline> pipeline) = 0;
		virtual void Unbind(Ref<Pipeline> pipeline) = 0;

		inline Ref<Shader> GetShader() { return m_Shader; }
		inline std::string GetName() const { return m_MaterialData.Name; }

		inline bool HasTexture(TextureType type) const { return !m_Textures.empty() && m_Textures.size() > type.Index && m_Textures[type.Index]->GetID() != 0; }
		inline Ref<Image2D> GetTexture(TextureType type) const { return m_Textures[type.Index]; }

		inline static const TextureType ALBEDO_TYPE = { aiTextureType_DIFFUSE, "Albedo", 1, 0 };
		inline static const TextureType NORMALS_TYPE = { aiTextureType_HEIGHT, "Normals", 2, 1 };
		inline static const TextureType METALLIC_TYPE = { aiTextureType_SHININESS, "Metallic", 3, 2 };
		inline static const TextureType ROUGHNESS_TYPE = { aiTextureType_SPECULAR, "Roughness", 4, 3 };
		inline static const TextureType AO_TYPE = { aiTextureType_AMBIENT_OCCLUSION, "Ambient Occlusion", 5, 4 };
	protected:
		virtual void LoadTexture(aiMaterial* aiMaterial, TextureSlot slot, TextureType type, std::string& importedFilePath) = 0;

		MaterialData m_MaterialData;
		Ref<Shader> m_Shader;
		std::vector<Ref<Image2D>> m_Textures;
	};
}


