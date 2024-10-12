#pragma once

#include "Renderer/Memory/Memory.h"

#include "Renderer/Device/RenderResource.h"
#include "Renderer/RenderPass.h"

#include "RenderGraphResource.h"

namespace Lucy {

	class Image;

	struct RGImageData {
		RenderResourceHandle ResourceHandle;
		RenderPassLoadStoreAttachments LoadStoreAttachment;
		bool IsDepth = false;
	};

	struct RGBufferData {
		RenderResourceHandle ResourceHandle;
		//Alignment? Any special formats? etc...
	};

	using ExternalResources = std::unordered_map<RenderGraphResource, RenderResourceHandle>;
	using RGImageResources = std::unordered_map<RenderGraphResource, RGImageData>;
	using RGBufferResources = std::unordered_map<RenderGraphResource, RGBufferData>;

	class RenderGraphRegistry final {
	public:
		RenderGraphRegistry(ExternalResources& externalResources, ExternalResources& externalTransientResources);
		~RenderGraphRegistry() = default;

		void DeclareImage(const RenderGraphResource& rgResource, const RGImageData& imageData);
		void DeclareBuffer(const RenderGraphResource& rgResource, const RGBufferData& bufferData);

		Ref<Image> GetImage(const RenderGraphResource& rgResource);
		Ref<Image> GetExternalImage(const RenderGraphResource& rgResource);
	private:
		const RGImageData& GetImageData(const RenderGraphResource& rgResource);
		const RGBufferData& GetBufferData(const RenderGraphResource& rgResource);

		RGImageResources m_ImageResources;
		RGBufferResources m_BufferResources;

		ExternalResources& m_ExternalResources;
		ExternalResources& m_ExternalTransientResources;

		friend class RenderGraph; //for GetImageData...
	};
}