#pragma once

#include <deque>

#include "Context/RenderContext.h"
#include "Commands/RenderCommandQueue.h"

namespace Lucy {

	class Window;
	class Image;
	class VulkanPushConstant;
	class DescriptorSet;

	class RenderDevice;
	class SwapChain;
	class RenderGraphPass;

	struct EntityPickedEvent;

	using RenderDeletionFunc = std::function<void()>;

	class RendererBackend : public MemoryTrackable {
	protected:
		uint32_t m_MaxFramesInFlight = 0;
		uint32_t m_ImageIndex = 0;
		uint32_t m_CurrentFrameIndex = 0;
	private:
		static Ref<RendererBackend> Create(RendererConfiguration config, const Ref<Window>& window);
	public:
		virtual ~RendererBackend() = default;
		
		void EnqueueToRenderCommandQueue(RenderCommandFunc&& func);
		void EnqueueResourceDestroy(RenderResourceHandle handle);

		void SubmitToRender(RenderGraphPass& pass, RenderResourceHandle renderPassHandle, RenderResourceHandle frameBufferHandle);
		void SubmitToCompute(RenderGraphPass& pass);
		virtual RenderContextResultCodes WaitAndPresent() = 0;

		virtual void ExecuteBarrier(void* commandBufferHandle, Ref<Image> image) = 0;
		virtual void ExecuteBarrier(void* commandBufferHandle, void* imageHandle, uint32_t imageLayout, uint32_t layerCount, uint32_t mipCount) = 0;

		virtual void Destroy();
		
		inline const RendererConfiguration& GetRendererConfig() const { return m_RendererConfiguration; }

		inline const RenderCommandQueueMetricsOutput& GetCommandQueueMetrics() const { return m_CommandQueueMetricsOutput; }

		inline uint32_t GetCurrentImageIndex() const { return m_ImageIndex; }
		inline uint32_t GetCurrentFrameIndex() const { return m_CurrentFrameIndex; }
		inline uint32_t GetMaxFramesInFlight() const { return m_MaxFramesInFlight; }

		virtual void OnWindowResize() = 0;
		virtual void OnViewportResize() = 0;
		virtual glm::vec3 OnMousePicking(const EntityPickedEvent& e, const Ref<Image>& currentFrameBufferImage) = 0;

		virtual void InitializeImGui() = 0;
		virtual void RTRenderImGui() = 0;
	protected:
		virtual void Init() = 0;

		virtual void BeginFrame() = 0;
		virtual void RenderFrame() = 0;
		virtual void EndFrame() = 0;

		void EnqueueToRenderCommandQueue(RenderSubmitFunc&& func);

		void RecreateCommandQueue();
		void FlushCommandQueue();
		void FlushSubmitQueue();

		virtual void FlushDeletionQueue() = 0;

		inline const Ref<RenderContext>& GetRenderContext() const { return m_Context; }
		inline const Ref<RenderDevice>& GetRenderDevice() const { return m_RenderDevice; }
		inline const Ref<SwapChain>& GetSwapChain() const { return m_SwapChain; }

		RendererBackend(RendererConfiguration config, const Ref<Window>& window);

		std::vector<std::deque<RenderDeletionFunc>> m_ResourceDeletionQueues;
	protected:
		Ref<RenderContext> m_Context = nullptr;
		Ref<RenderDevice> m_RenderDevice = nullptr;
		Ref<SwapChain> m_SwapChain = nullptr;

		Ref<RenderCommandQueue> m_RenderCommandQueue = nullptr;
		Ref<RenderCommandQueue> m_RenderComputeCommandQueue = nullptr;

		RendererConfiguration m_RendererConfiguration;

		RenderCommandQueueMetricsOutput m_CommandQueueMetricsOutput;
		RenderCommandQueueMetricsOutput m_CommandQueueMetricsOutputCompute;

		friend class Renderer;
		friend class RenderThread;
		friend class ImGuiVulkanImpl;
	};
}