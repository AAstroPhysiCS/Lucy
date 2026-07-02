#pragma once

#include "Material.h"

#include <glm/fwd.hpp>

namespace Lucy {

	struct PBRMaterialData {
		glm::vec3 Albedo = glm::vec3(1.0f);
		float Metallic = 1.0f, Roughness = 1.0f, AOContribution = 1.0f, NormalStrength = 1.0f;

		PBRMaterialData(glm::vec3 albedo, float metallic, float roughness, float aoContribution)
			: Albedo(albedo), Metallic(metallic), Roughness(roughness), AOContribution(aoContribution) {
		}
		PBRMaterialData() = default;
	};

	struct alignas(16) PBRMaterialDataGPU {
		float AlbedoSlot = -1.0f;
		float NormalSlot = -1.0f;
		float RoughnessSlot = -1.0f;
		float MetallicSlot = -1.0f;

		glm::vec4 BaseAlbedoColor = glm::vec4(1.0f);
		float BaseRoughnessValue = 0.0f;
		float BaseMetallicValue = 0.0f;
		float BaseAOValue = 1.0f;
		float AOSlot = -1.0f;
		float NormalStrength = 1.0f;
	};

	class PBRMaterial final : public Material {
	public:
		PBRMaterial(const MaterialCreateInfo& createInfo, const PBRMaterialData& data);
		virtual ~PBRMaterial() = default;

		inline glm::vec3& GetAlbedoColor() { return m_MaterialData.Albedo; }
		inline float& GetRoughnessValue() { return m_MaterialData.Roughness; }
		inline float& GetMetallicValue() { return m_MaterialData.Metallic; }
		inline float& GetAOContribution() { return m_MaterialData.AOContribution; }
		inline float& GetNormalStrength() { return m_MaterialData.NormalStrength; }

		void SetTexture(const MaterialImageType& type, RenderResourceHandle textureHandle);

		void Update() final override;
		void RTDestroyResource() final override;

		static inline const MaterialImageType ALBEDO_TYPE = { "Albedo", 0 };
		static inline const MaterialImageType NORMALS_TYPE = { "Normals", 1 };
		static inline const MaterialImageType METALLIC_TYPE = { "Metallic", 2 };
		static inline const MaterialImageType ROUGHNESS_TYPE = { "Roughness", 3 };
		static inline const MaterialImageType AO_TYPE = { "Ambient Occlusion", 4 };
	private:
		PBRMaterialDataGPU m_MaterialShaderData;
		PBRMaterialData m_MaterialData;
	};
}