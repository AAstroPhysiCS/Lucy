#include "lypch.h"
#include "PBRMaterial.h"

#include "Renderer/Renderer.h"
#include "Renderer/Image/Image.h"

namespace Lucy {

	PBRMaterial::PBRMaterial(const MaterialCreateInfo& createInfo) 
		: Material(createInfo) {
	}

	void PBRMaterial::SetTexture(const MaterialImageType& type, RenderDeviceResourceHandle textureHandle) {
		AddTexture(type.Index, textureHandle);
	}

	std::any PBRMaterial::BuildRenderData(const Ref<RenderDevice>& device) {
		LUCY_PROFILE_NEW_EVENT("PBRMaterial::BuildRenderData");

		const auto& pipeline = Renderer::GetPipelineManager()->GetAs<GraphicsPipeline>("PBRGeometryPipeline");

		const auto& albedo = HasImage(PBRMaterial::ALBEDO_TYPE) ? GetImage(PBRMaterial::ALBEDO_TYPE) : nullptr;
		const auto& normals = HasImage(PBRMaterial::NORMALS_TYPE) ? GetImage(PBRMaterial::NORMALS_TYPE) : nullptr;
		const auto& metallic = HasImage(PBRMaterial::METALLIC_TYPE) ? GetImage(PBRMaterial::METALLIC_TYPE) : nullptr;
		const auto& roughness = HasImage(PBRMaterial::ROUGHNESS_TYPE) ? GetImage(PBRMaterial::ROUGHNESS_TYPE) : nullptr;
		const auto& ao = HasImage(PBRMaterial::AO_TYPE) ? GetImage(PBRMaterial::AO_TYPE) : nullptr;
		//const auto& orm = HasImage(PBRMaterial::ORM_TYPE) ? GetImage(PBRMaterial::ORM_TYPE) : nullptr;

		const auto BindTexture = [&](const Ref<Image>& image, RenderDeviceTextureResource& resource) {
			if (!image) {
				resource.TextureIndex = INVALID_INDEX;
				resource.SamplerIndex = INVALID_INDEX;
				return;
			}

			resource.TextureIndex = device->BindGlobalImageHandleTo("Textures2D", pipeline, image, -1);
			resource.SamplerIndex = 0;
		};

		BindTexture(albedo, m_MaterialData.AlbedoMap);
		BindTexture(normals, m_MaterialData.NormalMap);
		BindTexture(metallic, m_MaterialData.MetallicMap);
		BindTexture(roughness, m_MaterialData.RoughnessMap);
		BindTexture(ao, m_MaterialData.AOMap);

		return m_MaterialData;
	}

    void PBRMaterial::RTDestroyResource() {
		for (RenderDeviceResourceHandle& imageHandle : GetAllTextureHandles())
			Renderer::EnqueueResourceDestroy(imageHandle);
	}
}