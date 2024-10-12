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

		void BeginFrame() final override;
		void RenderFrame() final override;
		void EndFrame() final override;

		Ref<Image> GetOutputImage() final override;
	};
}