#pragma once

#include "RenderPipeline.h"
#include "Renderer/Pipeline/GraphicsPipeline.h"

namespace Lucy {

	struct WindowResizeEvent;
	struct SwapChainResizeEvent;

	class GPUDrivenRenderPipeline final : public RenderPipeline {
	public:
		GPUDrivenRenderPipeline(const RenderPipelineCreateInfo& createInfo, const Ref<Scene>& scene);
		virtual ~GPUDrivenRenderPipeline() = default;

		GPUDrivenRenderPipeline(const GPUDrivenRenderPipeline&) = delete;
		GPUDrivenRenderPipeline& operator=(const GPUDrivenRenderPipeline&) = delete;
		GPUDrivenRenderPipeline(GPUDrivenRenderPipeline&&) = delete;
		GPUDrivenRenderPipeline& operator=(GPUDrivenRenderPipeline&&) = delete;

		void BeginFrame(const Ref<RenderDevice>& device, Ref<Scene>& scene) final override;
		void RenderFrame() final override;
		void EndFrame() final override;

		Ref<Image> GetOutputImage() final override;
	private:
		RenderDeviceObjectHandle m_MainCullViewHandle{};
	};
}