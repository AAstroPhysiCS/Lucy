#pragma once

#include "Material.h"

#include <glm/fwd.hpp>

namespace Lucy {

	struct PBRMaterialData {
		glm::vec3 Diffuse = glm::vec3();
		float Metallic = 0.0f, Roughness = 0.0f, AOContribution = 0.0f;
		std::vector<RenderResourceHandle> TextureHandles;

		PBRMaterialData(glm::vec3 diffuse, float metallic, float roughness, float aoContribution)
			: Diffuse(diffuse), Metallic(metallic), Roughness(roughness), AOContribution(aoContribution) {
		}
		PBRMaterialData() = default;
	};

	struct PBRMaterialShaderData {
		float AlbedoSlot = -1.0f;
		float NormalSlot = -1.0f;
		float RoughnessSlot = -1.0f;
		float MetallicSlot = -1.0f;

		glm::vec4 BaseDiffuseColor = glm::vec4(0.0f);
		float BaseRoughnessValue = 0.0f;
		float BaseMetallicValue = 0.0f;
		float BaseAOValue = 0.0f;
		float AOSlot = -1.0f;
	};

	struct PBRMaterialImageType {
		std::string Name;
		uint32_t Index;
	};

	class PBRMaterial final : public Material {
	public:
		PBRMaterial(MaterialID materialID, const Ref<Shader>& shader, const PBRMaterialData& data);
		virtual ~PBRMaterial() = default;

		inline float& GetRoughnessValue() { return m_MaterialData.Roughness; }
		inline float& GetMetallicValue() { return m_MaterialData.Metallic; }
		inline float& GetAOContribution() { return m_MaterialData.AOContribution; }

		inline Ref<Image> GetImage(const PBRMaterialImageType& type) const { 
			return Renderer::AccessResource<Image>(m_MaterialData.TextureHandles[type.Index]); 
		}

		inline bool HasImage(const PBRMaterialImageType& type) const {
			return !m_MaterialData.TextureHandles.empty() && 
				m_MaterialData.TextureHandles.size() > type.Index && 
				m_MaterialData.TextureHandles[type.Index] != InvalidRenderResourceHandle;
		}

		void Update() final override;
		void RTDestroyResource() final override;
		void AddTexture(RenderResourceHandle textureHandle);

		static inline const PBRMaterialImageType ALBEDO_TYPE = { "Albedo", 0 };
		static inline const PBRMaterialImageType NORMALS_TYPE = { "Normals", 1 };
		static inline const PBRMaterialImageType METALLIC_TYPE = { "Metallic", 2 };
		static inline const PBRMaterialImageType ROUGHNESS_TYPE = { "Roughness", 3 };
		static inline const PBRMaterialImageType AO_TYPE = { "Ambient Occlusion", 4 };
	private:
		Ref<Shader> m_PBRShader = nullptr;

		PBRMaterialShaderData m_MaterialShaderData;
		PBRMaterialData m_MaterialData;
	};
}