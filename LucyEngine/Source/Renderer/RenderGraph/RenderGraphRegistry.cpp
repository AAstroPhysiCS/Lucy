#include "lypch.h"
#include "RenderGraphRegistry.h"

#include "Renderer/Renderer.h"

namespace Lucy {
	
	void RenderGraphRegistry::Flush() {
		for (auto& [rgResource, entry] : m_Resources) {
			if (!Renderer::IsValidRenderResource(entry.ResourceHandle) || !entry.IsExternalTransient())
				continue;
			Renderer::EnqueueResourceDestroy(entry.ResourceHandle);
		}
	}

	void RenderGraphRegistry::ImportExternalResource(const RenderGraphResource& rgResource, RenderResourceHandle handle) {
		m_Resources.insert_or_assign(rgResource,
			RGResourceEntry{
				.ResourceHandle = handle,
				.Type = RGResourceType::External,
				.Data = {},
			}
		);
	}

	void RenderGraphRegistry::ImportExternalTransientResource(const RenderGraphResource& rgResource, RenderResourceHandle handle) {
		m_Resources.insert_or_assign(rgResource, 
			RGResourceEntry {
				.ResourceHandle = handle,
				.Type = RGResourceType::ExternalTransient,
				.Data = {},
			}
		);
	}

	bool RenderGraphRegistry::Contains(const RenderGraphResource& rgResource) const {
		return m_Resources.contains(rgResource);
	}

	void RenderGraphRegistry::DeclareImage(const RenderGraphResource& rgResource, RenderResourceHandle handle, const RGImageData& imageData) {
		LUCY_ASSERT(rgResource != UndefinedRenderGraphResource);
		LUCY_ASSERT(Renderer::IsValidRenderResource(handle));

		m_Resources.insert_or_assign(
			rgResource,
			RGResourceEntry {
				.ResourceHandle = handle,
				.Type = RGResourceType::Internal,
				.Data = imageData
			}
		);
	}

	void RenderGraphRegistry::DeclareBuffer(const RenderGraphResource& rgResource, RenderResourceHandle handle, const RGBufferData& bufferData) {
		LUCY_ASSERT(rgResource != UndefinedRenderGraphResource);
		LUCY_ASSERT(Renderer::IsValidRenderResource(handle));

		m_Resources.insert_or_assign(
			rgResource,
			RGResourceEntry {
				.ResourceHandle = handle,
				.Type = RGResourceType::Internal,
				.Data = bufferData
			}
		);
	}

	Ref<Image> RenderGraphRegistry::GetImage(const RenderGraphResource& rgResource) {
		return Renderer::AccessResource<Image>(m_Resources.at(rgResource).ResourceHandle);
	}
	
	Ref<Image> RenderGraphRegistry::GetImage(const RenderGraphResource& rgResource) const {
		return Renderer::AccessResource<Image>(m_Resources.at(rgResource).ResourceHandle);
	}

	Ref<RenderResource> RenderGraphRegistry::GetBuffer(const RenderGraphResource& rgResource) {
		return Renderer::AccessResource<RenderResource>(m_Resources.at(rgResource).ResourceHandle);
	}

	Ref<RenderResource> RenderGraphRegistry::GetBuffer(const RenderGraphResource& rgResource) const {
		return Renderer::AccessResource<RenderResource>(m_Resources.at(rgResource).ResourceHandle);
	}
}