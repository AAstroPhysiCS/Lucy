#pragma once

#include "Context/RenderContext.h"
#include "Commands/CommandQueue.h"

#include "RenderThread.h"

namespace Lucy {

	class Window;
	class Image;
	class VulkanPushConstant;
	class DescriptorSet;

	class RenderGraphPass;

	struct EntityPickedEvent;

	using RenderDeletionFunc = std::function<void()>;

	class RendererBackend : public MemoryTrackable {
	protected:
		uint32_t m_MaxFramesInFlight = 0;
		uint32_t m_ImageIndex = 0;
		uint32_t m_CurrentFrameIndex = 0;
	public:
		static Ref<RendererBackend> Create(RendererConfiguration config, const Ref<Window>& window);
		virtual ~RendererBackend() = default;
		
		void EnqueueToRenderThread(RenderCommandFunc&& func);
		void EnqueueResourceDestroy(RenderResourceHandle& handle);

		void SubmitToRender(RenderGraphPass& pass, RenderResourceHandle renderPassHandle, RenderResourceHandle frameBufferHandle);
		void SubmitToCompute(RenderGraphPass& pass);
		virtual RenderContextResultCodes WaitAndPresent() = 0;

		virtual void ExecuteBarrier(void* commandBufferHandle, Ref<Image> image) = 0;
		virtual void ExecuteBarrier(void* commandBufferHandle, void* imageHandle, uint32_t imageLayout, uint32_t layerCount, uint32_t mipCount) = 0;

		virtual void Destroy();
		
		inline const Ref<RenderContext>& GetRenderContext() const { return m_RenderContext; }
		inline const RendererConfiguration& GetRendererConfig() const { return m_RendererConfiguration; }

		inline const CommandQueueMetricsOutput& GetCommandQueueMetrics() const { return m_CommandQueueMetricsOutput; }

		inline uint32_t GetCurrentImageIndex() const { return m_ImageIndex; }
		inline uint32_t GetCurrentFrameIndex() const { return m_CurrentFrameIndex; }
		inline uint32_t GetMaxFramesInFlight() const { return m_MaxFramesInFlight; }

		virtual void OnWindowResize() = 0;
		virtual void OnViewportResize() = 0;
		virtual glm::vec3 OnMousePicking(const EntityPickedEvent& e, const Ref<Image>& currentFrameBufferImage) = 0;

		virtual void InitializeImGui() = 0;
		virtual void RenderImGui() = 0;
	protected:
		virtual void Init();

		virtual void BeginFrame() = 0;
		virtual void RenderFrame() = 0;
		virtual void EndFrame() = 0;

		void EnqueueToRenderThread(RenderSubmitFunc&& func);

		void RecreateCommandQueue();
		void FlushCommandQueue();
		void FlushSubmitQueue();
		void FlushDeletionQueue();

		inline const std::vector<RenderCommandList>& GetCommandLists() const { return m_CommandQueue->GetCommandLists(); }
		inline const Ref<RenderDevice>& GetRenderDevice() const { return m_RenderContext->GetRenderDevice(); }
		inline const Ref<RenderThread>& GetRenderThread() const { return m_RenderThread; }

		RendererBackend(RendererConfiguration config, const Ref<Window>& window);
	private:
		Ref<RenderContext> m_RenderContext = nullptr;
		Ref<RenderThread> m_RenderThread = nullptr;
		Ref<CommandQueue> m_CommandQueue = nullptr;
		RendererConfiguration m_RendererConfiguration;

		std::vector<RenderDeletionFunc> m_ResourceDeletionQueue;

		CommandQueueMetricsOutput m_CommandQueueMetricsOutput;

		friend class Renderer;
	};
}