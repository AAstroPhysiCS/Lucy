#pragma once

#include "RenderPipeline.h"
#include "Renderer/Pipeline/GraphicsPipeline.h"

namespace Lucy {

	struct WindowResizeEvent;
	struct SwapChainResizeEvent;

	class ViewportRenderPipeline final : public RenderPipeline {
	public:
		ViewportRenderPipeline(const RenderPipelineCreateInfo& createInfo, const Ref<Scene>& scene);
		virtual ~ViewportRenderPipeline() = default;

		ViewportRenderPipeline(const ViewportRenderPipeline&) = delete;
		ViewportRenderPipeline& operator=(const ViewportRenderPipeline&) = delete;
		ViewportRenderPipeline(ViewportRenderPipeline&&) = delete;
		ViewportRenderPipeline& operator=(ViewportRenderPipeline&&) = delete;

		void BeginFrame(const Ref<RenderDevice>& device, Ref<Scene>& scene) final override;
		void RenderFrame() final override;
		void EndFrame() final override;

		Ref<Image> GetOutputImage() final override;
	};
}