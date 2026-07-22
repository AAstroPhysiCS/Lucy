#pragma once

namespace Lucy {

	class RenderGraph;
	class RenderGraphPass;
	class RenderGraphResource;

	enum class RenderGraphResourceAccess : uint8_t;
	enum class RenderGraphExecutionPolicy : uint8_t;

	struct ImageCreateInfo;

	class RenderGraphBuilder final {
	public:
		RenderGraphBuilder(RenderGraph* renderGraph, RenderGraphPass* pass);
		~RenderGraphBuilder() = default;

		RenderGraphBuilder(const RenderGraphBuilder& other) = delete;
		RenderGraphBuilder(RenderGraphBuilder&& other) noexcept = delete;
		RenderGraphBuilder& operator=(const RenderGraphBuilder& other) = delete;
		RenderGraphBuilder& operator=(RenderGraphBuilder&& other) noexcept = delete;

		void SetViewportArea(uint32_t width, uint32_t height);
		void SetInFlightMode(bool mode);
		void SetClearColor(ClearColor clearColor);
		void SetExecutionPolicy(RenderGraphExecutionPolicy policy);

		void DeclareImage(const RenderGraphResource& rgResource, const ImageCreateInfo& createInfo, RenderPassLoadStoreAttachments loadStoreAccessOp);
		void DeclareImage(const RenderGraphResource& rgResource, const ImageCreateInfo& createInfo, RenderPassLoadStoreAttachments loadStoreAccessOp,
						const RenderGraphResource& rgResourceDepth, const ImageCreateInfo& createDepthInfo, RenderPassLoadStoreAttachments loadStoreDepthAccessOp);

		void BindRenderTarget(const RenderGraphResource& rgResourceToBind, const RenderGraphResource& rgResourceDepthToBind);
		void BindRenderTarget(const RenderGraphResource& rgResourceToBind);

		void ReadExternalImage(const RenderGraphResource& rgResource, RenderGraphResourceAccess access);
		void ReadExternalTransientImage(const RenderGraphResource& rgResource, RenderGraphResourceAccess access);
		void WriteExternalImage(const RenderGraphResource& rgResource, RenderGraphResourceAccess access);
#pragma region Compute
		void ReadBuffer(const RenderGraphResource& rgResourceToRead, RenderGraphResourceAccess access);
		void WriteBuffer(const RenderGraphResource& rgResourceToWrite, RenderGraphResourceAccess access);

		void ReadImage(const RenderGraphResource& rgResourceToRead, RenderGraphResourceAccess access);
		void WriteImage(const RenderGraphResource& rgResourceToWrite, RenderGraphResourceAccess access);
#pragma endregion Compute
	private:
		RenderGraph* m_RenderGraph = nullptr;
		RenderGraphPass* m_RenderGraphPass = nullptr;
	};
}