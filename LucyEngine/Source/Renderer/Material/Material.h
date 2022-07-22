#pragma once

#include "Core/Base.h"
#include "Core/FileSystem.h"

#include "glm/glm.hpp"
#include "assimp/scene.h"

#include "Renderer/Shader/Shader.h"
#include "Renderer/Image/Image.h"

#include "Utils/UUID.h"

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

	struct MaterialShaderData {
		float AlbedoSlot = -1.0f;
		float NormalSlot = -1.0f;
		float RoughnessSlot = -1.0f;
		float MetallicSlot = -1.0f;

		glm::vec4 BaseDiffuseColor = glm::vec4(0.0f);
		float Shininess = 0.0f;
		float Roughness = 0.0f;
		float Reflectivity = 0.0f;
		float AOSlot = -1.0f;
	};

	class Material {
	public:
		static Ref<Material> Create(Ref<Shader> shader, aiMaterial* aiMaterial, const char* submeshName, const std::string& importedFilePath);

		Material(Ref<Shader> shader, aiMaterial* aiMaterial, const char* submeshName, const std::string& importedFilePath);
		virtual ~Material() = default;

		inline Ref<Shader> GetShader() { return m_Shader; }
		inline const std::string& GetName() const { return m_MaterialData.Name; }
		inline LucyID GetID() const { return m_MaterialID; }

		void SetImage(MaterialImageType type, Ref<Image2D> image);
		inline Ref<Image2D> GetImage(MaterialImageType type) const { return m_Textures[type.Index]; }
		inline bool HasImage(MaterialImageType type) const {
			return !m_Textures.empty() && m_Textures.size() > type.Index 
				&& m_Textures[type.Index]->GetWidth() > 0 && m_Textures[type.Index]->GetHeight() > 0 && m_Textures[type.Index]->GetImGuiID() != 0;
		}
		
		virtual void Update(Ref<Pipeline> pipeline) = 0;
		void Destroy();

		inline static const MaterialImageType ALBEDO_TYPE = { aiTextureType_DIFFUSE, "Albedo", 0 };
		inline static const MaterialImageType NORMALS_TYPE = { aiTextureType_HEIGHT, "Normals", 1 };
		inline static const MaterialImageType METALLIC_TYPE = { aiTextureType_SHININESS, "Metallic", 2 };
		inline static const MaterialImageType ROUGHNESS_TYPE = { aiTextureType_SPECULAR, "Roughness", 3 };
		inline static const MaterialImageType AO_TYPE = { aiTextureType_AMBIENT_OCCLUSION, "Ambient Occlusion", 4 };
	protected:
		virtual void LoadTexture(aiMaterial* aiMaterial, MaterialImageType type, const std::string& importedFilePath) = 0;

		MaterialShaderData m_MaterialShaderData;
		MaterialData m_MaterialData;
		Ref<Shader> m_Shader;
		std::vector<Ref<Image2D>> m_Textures;
	private:
		inline static IDProvider s_MaterialIDProvider;
		LucyID m_MaterialID = s_MaterialIDProvider.RequestID();
	};
}