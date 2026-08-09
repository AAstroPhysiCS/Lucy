#include "lypch.h"
#include "RenderGraphRegistry.h"

#include "Renderer/Renderer.h"
#include "Renderer/Image/Image.h"

namespace Lucy {
	
	void RenderGraphRegistry::Flush() {
		for (auto& [rgResource, entry] : m_Resources) {
			if (!entry.IsExternalTransient())
				continue;

			for (auto& handle : entry.ResourceHandles) {
				if (!Renderer::IsValidRenderResource(handle))
					continue;
				Renderer::EnqueueResourceDestroy(handle);
			}
		}
	}

	void RenderGraphRegistry::ImportExternalResource(const RenderGraphResource& rgResource, RenderDeviceResourceHandle handle, RGResourceData data) {
		m_Resources.insert_or_assign(rgResource,
			RGResourceEntry{
				.ResourceHandles = { handle },
				.Type = RGResourceType::External,
				.Data = data,
			}
		);
	}

	void RenderGraphRegistry::ImportExternalResource(const RenderGraphResource& rgResource, const std::vector<RenderDeviceResourceHandle>& handles, RGResourceData data) {
		m_Resources.insert_or_assign(rgResource,
			RGResourceEntry{
				.ResourceHandles = handles,
				.Type = RGResourceType::External,
				.Data = data,
			}
		);
	}

	void RenderGraphRegistry::ImportExternalTransientResource(const RenderGraphResource& rgResource, RenderDeviceResourceHandle handle) {
		m_Resources.insert_or_assign(rgResource, 
			RGResourceEntry {
				.ResourceHandles = { handle },
				.Type = RGResourceType::ExternalTransient,
				.Data = {},
			}
		);
	}

	bool RenderGraphRegistry::Contains(const RenderGraphResource& rgResource) const {
		return m_Resources.contains(rgResource);
	}

	void RenderGraphRegistry::DeclareImage(const RenderGraphResource& rgResource, RenderDeviceResourceHandle handle, const RGImageData& imageData) {
		LUCY_ASSERT(rgResource != UndefinedRenderGraphResource);
		LUCY_ASSERT(Renderer::IsValidRenderResource(handle));

		m_Resources.insert_or_assign(
			rgResource,
			RGResourceEntry {
				.ResourceHandles = { handle },
				.Type = RGResourceType::Internal,
				.Data = imageData
			}
		);
	}

	void RenderGraphRegistry::DeclareImage(const RenderGraphResource& rgResource, const std::vector<RenderDeviceResourceHandle>& handles, const RGImageData& imageData) {
		LUCY_ASSERT(rgResource != UndefinedRenderGraphResource);
		for (const auto& handle : handles)
			LUCY_ASSERT(Renderer::IsValidRenderResource(handle));

		m_Resources.insert_or_assign(
			rgResource,
			RGResourceEntry{
				.ResourceHandles = handles,
				.Type = RGResourceType::Internal,
				.Data = imageData
			}
		);
	}

	void RenderGraphRegistry::DeclareBuffer(const RenderGraphResource& rgResource, RenderDeviceResourceHandle handle, const RGBufferData& bufferData) {
		LUCY_ASSERT(rgResource != UndefinedRenderGraphResource);
		LUCY_ASSERT(Renderer::IsValidRenderResource(handle));

		m_Resources.insert_or_assign(
			rgResource,
			RGResourceEntry {
				.ResourceHandles = { handle },
				.Type = RGResourceType::Internal,
				.Data = bufferData
			}
		);
	}

	void RenderGraphRegistry::DeclareBuffer(const RenderGraphResource& rgResource, const std::vector<RenderDeviceResourceHandle>& handles, const RGBufferData& bufferData) {
		LUCY_ASSERT(rgResource != UndefinedRenderGraphResource);
		for (const auto& handle : handles)
			LUCY_ASSERT(Renderer::IsValidRenderResource(handle));

		m_Resources.insert_or_assign(
			rgResource,
			RGResourceEntry{
				.ResourceHandles = handles,
				.Type = RGResourceType::Internal,
				.Data = bufferData
			}
		);
	}

	Ref<Image> RenderGraphRegistry::GetImage(const RenderGraphResource& rgResource) {
		return Renderer::AccessResource<Image>(m_Resources.at(rgResource).ResourceHandles[0]);
	}
	
	Ref<Image> RenderGraphRegistry::GetImage(const RenderGraphResource& rgResource) const {
		return Renderer::AccessResource<Image>(m_Resources.at(rgResource).ResourceHandles[0]);
	}

	Ref<RenderDeviceBuffer> RenderGraphRegistry::GetBuffer(const RenderGraphResource& rgResource) {
		const auto& data = m_Resources.at(rgResource).GetBufferData();
		if (!data.InFlightMode)
			return Renderer::AccessResource<RenderDeviceBuffer>(m_Resources.at(rgResource).ResourceHandles[0]);
		return Renderer::AccessResource<RenderDeviceBuffer>(m_Resources.at(rgResource).ResourceHandles[Renderer::GetCurrentFrameIndex()]);
	}

	Ref<RenderDeviceBuffer> RenderGraphRegistry::GetBuffer(const RenderGraphResource& rgResource) const {
		const auto& data = m_Resources.at(rgResource).GetBufferData();
		if (!data.InFlightMode)
			return Renderer::AccessResource<RenderDeviceBuffer>(m_Resources.at(rgResource).ResourceHandles[0]);
		return Renderer::AccessResource<RenderDeviceBuffer>(m_Resources.at(rgResource).ResourceHandles[Renderer::GetCurrentFrameIndex()]);
	}
}