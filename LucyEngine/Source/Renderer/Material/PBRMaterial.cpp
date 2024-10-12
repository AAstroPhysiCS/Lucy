#include "lypch.h"
#include "PBRMaterial.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	PBRMaterial::PBRMaterial(MaterialID materialID, const Ref<Shader>& shader, const PBRMaterialData& data) 
		: Material(materialID), m_PBRShader(shader), m_MaterialData(data) {
	}

	void PBRMaterial::Update() {
		LUCY_PROFILE_NEW_EVENT("Material::Update");

		if (HasImage(PBRMaterial::ALBEDO_TYPE)) {
			uint32_t pos = m_PBRShader->BindImageHandleTo("u_Textures", GetImage(PBRMaterial::ALBEDO_TYPE));
			m_MaterialShaderData.AlbedoSlot = pos;
		} else if (HasImage(PBRMaterial::NORMALS_TYPE)) {
			uint32_t pos = m_PBRShader->BindImageHandleTo("u_Textures", GetImage(PBRMaterial::NORMALS_TYPE));
			m_MaterialShaderData.NormalSlot = pos;
		} else if (HasImage(PBRMaterial::METALLIC_TYPE)) {
			uint32_t pos = m_PBRShader->BindImageHandleTo("u_Textures", GetImage(PBRMaterial::METALLIC_TYPE));
			m_MaterialShaderData.MetallicSlot = pos;
		} else if (HasImage(PBRMaterial::ROUGHNESS_TYPE)) {
			uint32_t pos = m_PBRShader->BindImageHandleTo("u_Textures", GetImage(PBRMaterial::ROUGHNESS_TYPE));
			m_MaterialShaderData.RoughnessSlot = pos;
		} else if (HasImage(PBRMaterial::AO_TYPE)) {
			uint32_t pos = m_PBRShader->BindImageHandleTo("u_Textures", GetImage(PBRMaterial::AO_TYPE));
			m_MaterialShaderData.AOSlot = pos;
		}

		//TODO: Change diffuse color to vec4
		m_MaterialShaderData.BaseDiffuseColor = glm::vec4(m_MaterialData.Diffuse, 1.0f);
		m_MaterialShaderData.BaseMetallicValue = m_MaterialData.Metallic;
		m_MaterialShaderData.BaseRoughnessValue = m_MaterialData.Roughness;
		m_MaterialShaderData.BaseAOValue = m_MaterialData.AOContribution;

		auto ssboMaterialAttributes = m_PBRShader->GetSharedStorageBufferIfExists("LucyMaterialAttributes");
		ssboMaterialAttributes->Append((uint8_t*)&m_MaterialShaderData, sizeof(m_MaterialShaderData));
	}

	void PBRMaterial::AddTexture(RenderResourceHandle textureHandle) {
		m_MaterialData.TextureHandles.push_back(textureHandle);
	}

	void PBRMaterial::RTDestroyResource() {
		for (RenderResourceHandle imageHandle : m_MaterialData.TextureHandles)
			Renderer::EnqueueResourceDestroy(imageHandle);
	}
}