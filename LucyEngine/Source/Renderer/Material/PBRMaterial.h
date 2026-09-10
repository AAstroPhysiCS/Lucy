#pragma once

#include "Material.h"

#include "Renderer/Device/RenderDeviceScene.h"

#include <glm/fwd.hpp>

namespace Lucy {

	class PBRMaterial final : public Material {
	public:
		PBRMaterial(const MaterialCreateInfo& createInfo);
		virtual ~PBRMaterial() = default;

		PBRMaterial(const PBRMaterial&) = delete;
		PBRMaterial& operator=(const PBRMaterial&) = delete;
		PBRMaterial(PBRMaterial&&) = delete;
		PBRMaterial& operator=(PBRMaterial&&) = delete;

		glm::vec4& GetAlbedoColor() { return m_MaterialData.BaseColor; }
		float& GetAOContribution() { return m_MaterialData.ORME.r; }
		float& GetRoughnessValue() { return m_MaterialData.ORME.g; }
		float& GetMetallicValue() { return m_MaterialData.ORME.b; }
		float& GetEmissiveValue() { return m_MaterialData.ORME.a; }
		float& GetNormalStrength() { return m_MaterialData.NormalStrength; }

		void SetTexture(const MaterialImageType& type, RenderDeviceResourceHandle textureHandle);

		std::any BuildRenderData(const Ref<RenderDevice>& device) final override;
		void RTDestroyResource() final override;

		static inline const MaterialImageType ALBEDO_TYPE = { "Albedo", 0 };
		static inline const MaterialImageType NORMALS_TYPE = { "Normals", 1 };
		static inline const MaterialImageType ORM_TYPE = { "ORM", 2 };
		static inline const MaterialImageType AO_TYPE = { "AO", 3 };
		static inline const MaterialImageType ROUGHNESS_TYPE = { "Roughness", 4 };
		static inline const MaterialImageType METALLIC_TYPE = { "Metallic", 5 };
		static inline const MaterialImageType EMISSIVE_TYPE = { "Emissive", 6 };
	private:

		RenderDevicePBRMaterialData m_MaterialData;
	};
}