#pragma once

namespace Lucy {

	class RenderGraph;
	class RenderGraphPass;
	class RenderGraphResource;

	struct ImageCreateInfo;

	class RenderGraphBuilder final {
	public:
		RenderGraphBuilder(RenderGraph* renderGraph, RenderGraphPass* pass);
		~RenderGraphBuilder() = default;

		void SetViewportArea(uint32_t width, uint32_t height);
		void SetInFlightMode(bool mode);

		void DeclareImage(const RenderGraphResource& rgResource, const ImageCreateInfo& createInfo, RenderPassLoadStoreAttachments loadStoreAccessOp);
		void DeclareImage(const RenderGraphResource& rgResource, const ImageCreateInfo& createInfo, RenderPassLoadStoreAttachments loadStoreAccessOp,
						const RenderGraphResource& rgResourceDepth, const ImageCreateInfo& createDepthInfo, RenderPassLoadStoreAttachments loadStoreDepthAccessOp);

		void ReadExternalImage(const RenderGraphResource& rgResource);
		void ReadExternalTransientImage(const RenderGraphResource& rgResource);
		void WriteExternalImage(const RenderGraphResource& rgResource);

		void BindRenderTarget(const RenderGraphResource& rgResourceToBind, const RenderGraphResource& rgResourceDepthToBind);
		void BindRenderTarget(const RenderGraphResource& rgResourceToBind);
#pragma region Compute
		void ReadBuffer(const RenderGraphResource& rgResourceToRead);
		void WriteBuffer(const RenderGraphResource& rgResourceToWrite);

		void ReadImage(const RenderGraphResource& rgResourceToRead);
		void WriteImage(const RenderGraphResource& rgResourceToWrite);
#pragma endregion Compute
	private:
		RenderGraph* m_RenderGraph = nullptr;
		RenderGraphPass* m_RenderGraphPass = nullptr;
	};
}