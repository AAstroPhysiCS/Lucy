#include "lypch.h"
#include "PBRMaterial.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	PBRMaterial::PBRMaterial(const MaterialCreateInfo& createInfo, const PBRMaterialData& data) 
		: Material(createInfo), m_MaterialData(data) {
	}

	void PBRMaterial::Update() {
		LUCY_PROFILE_NEW_EVENT("Material::Update");

		const Ref<Pipeline>& pbrPipeline = GetPipeline();

		if (HasImage(PBRMaterial::ALBEDO_TYPE)) {
			uint32_t pos = pbrPipeline->BindImageHandleTo("u_Textures", GetImage(PBRMaterial::ALBEDO_TYPE));
			m_MaterialShaderData.AlbedoSlot = pos;
		} 
		
		if (HasImage(PBRMaterial::NORMALS_TYPE)) {
			uint32_t pos = pbrPipeline->BindImageHandleTo("u_Textures", GetImage(PBRMaterial::NORMALS_TYPE));
			m_MaterialShaderData.NormalSlot = pos;
		} 
		
		if (HasImage(PBRMaterial::METALLIC_TYPE)) {
			uint32_t pos = pbrPipeline->BindImageHandleTo("u_Textures", GetImage(PBRMaterial::METALLIC_TYPE));
			m_MaterialShaderData.MetallicSlot = pos;
		} 
		
		if (HasImage(PBRMaterial::ROUGHNESS_TYPE)) {
			uint32_t pos = pbrPipeline->BindImageHandleTo("u_Textures", GetImage(PBRMaterial::ROUGHNESS_TYPE));
			m_MaterialShaderData.RoughnessSlot = pos;
		} 
		
		if (HasImage(PBRMaterial::AO_TYPE)) {
			uint32_t pos = pbrPipeline->BindImageHandleTo("u_Textures", GetImage(PBRMaterial::AO_TYPE));
			m_MaterialShaderData.AOSlot = pos;
		}

		//TODO: Change diffuse color to vec4
		m_MaterialShaderData.BaseAlbedoColor = glm::vec4(m_MaterialData.Albedo, 1.0f);
		m_MaterialShaderData.BaseMetallicValue = m_MaterialData.Metallic;
		m_MaterialShaderData.BaseRoughnessValue = m_MaterialData.Roughness;
		m_MaterialShaderData.BaseAOValue = m_MaterialData.AOContribution;
		m_MaterialShaderData.NormalStrength = m_MaterialData.NormalStrength;

		auto ssboMaterialAttributes = pbrPipeline->GetSharedStorageBufferIfExists("MaterialAttributes");
		ssboMaterialAttributes->Append((uint8_t*)&m_MaterialShaderData, sizeof(m_MaterialShaderData));
	}

	void PBRMaterial::SetTexture(const MaterialImageType& type, RenderResourceHandle textureHandle) {
		AddTexture(type.Index, textureHandle);
	}

	void PBRMaterial::RTDestroyResource() {
		for (RenderResourceHandle imageHandle : GetAllTextureHandles())
			Renderer::EnqueueResourceDestroy(imageHandle);
	}
}