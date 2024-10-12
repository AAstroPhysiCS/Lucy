#include "lypch.h"
#include "RenderGraphRegistry.h"

#include "Renderer/Renderer.h"

namespace Lucy {
	
	RenderGraphRegistry::RenderGraphRegistry(ExternalResources& externalResources, ExternalResources& externalTransientResources)
		: m_ExternalResources(externalResources), m_ExternalTransientResources(externalTransientResources) {
	}

	void RenderGraphRegistry::DeclareImage(const RenderGraphResource& rgResource, const RGImageData& imageData) {
		LUCY_ASSERT(rgResource != UndefinedRenderGraphResource);
		LUCY_ASSERT(Renderer::IsValidRenderResource(imageData.ResourceHandle));

		m_ImageResources.try_emplace(rgResource, imageData);
	}

	void RenderGraphRegistry::DeclareBuffer(const RenderGraphResource& rgResource, const RGBufferData& bufferData) {
		LUCY_ASSERT(rgResource != UndefinedRenderGraphResource);
		LUCY_ASSERT(Renderer::IsValidRenderResource(bufferData.ResourceHandle));

		m_BufferResources.try_emplace(rgResource, bufferData);
	}

	Ref<Image> RenderGraphRegistry::GetImage(const RenderGraphResource& rgResource) {
		const auto GetImageHandle = [&]() {
			return m_ImageResources.at(rgResource).ResourceHandle;
		};
		return Renderer::AccessResource<Image>(GetImageHandle());
	}
	
	Ref<Image> RenderGraphRegistry::GetExternalImage(const RenderGraphResource& rgResource) {
		const auto GetExternalImageHandle = [&]() {
			if (m_ExternalResources.contains(rgResource))
				return m_ExternalResources.at(rgResource);
			return m_ExternalTransientResources.at(rgResource);
		};
		return Renderer::AccessResource<Image>(GetExternalImageHandle());
	}
	
	const RGImageData& RenderGraphRegistry::GetImageData(const RenderGraphResource& rgResource) {
		return m_ImageResources.at(rgResource);
	}

	const RGBufferData& RenderGraphRegistry::GetBufferData(const RenderGraphResource& rgResource) {
		return m_BufferResources.at(rgResource);
	}
}