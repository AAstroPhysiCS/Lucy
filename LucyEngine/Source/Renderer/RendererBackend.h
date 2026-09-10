#pragma once

#include <deque>

#include "Context/RenderContext.h"
#include "Commands/RenderCommandQueue.h"

namespace Lucy {

	class Window;
	class Image;
	class PipelineConstant;
	class DescriptorSet;

	class RenderDevice;
	class SwapChain;
	class RenderGraphPass;

	struct ExecutionBatch;
	struct RenderFrameHandles;

	struct EntityPickedEvent;

	using RenderDeletionFunc = std::function<void(const Ref<RenderDevice>& device)>;
	using RenderRecreateFunc = std::function<RenderDeletionFunc(const Ref<RenderDevice>&)>;

	class RendererBackend : public MemoryTrackable {
	protected:
		uint32_t m_MaxFramesInFlight = 0;
		uint32_t m_ImageIndex = 0;
		uint32_t m_CurrentFrameIndex = 0;

		uint64_t m_FrameNumber = 0;
	private:
		static Ref<RendererBackend> Create(RendererConfiguration config, const Ref<Window>& window);
	public:
		virtual ~RendererBackend() = default;

		RendererBackend(const RendererBackend& other) = delete;
		RendererBackend(RendererBackend&& other) noexcept = delete;
		RendererBackend& operator=(const RendererBackend& other) = delete;
		RendererBackend& operator=(RendererBackend&& other) noexcept = delete;
		
		void EnqueueToRenderCommandQueue(RenderCommandFunc&& func);

		void EnqueueResourceDestroy(RenderDeviceResourceHandle handle);
		void EnqueueResourceDestroy(RenderDeletionFunc&& func);
		void EnqueueResourceRecreate(RenderRecreateFunc&& func);

		virtual void SubmitBatchesToRender(std::vector<ExecutionBatch>& batches, const std::unordered_map<std::string, RenderFrameHandles>& renderFrameHandleMap) = 0;
		virtual RenderContextResultCodes WaitAndPresent() = 0;

		virtual void Destroy();
		
		const RendererConfiguration& GetRendererConfig() const { return m_RendererConfiguration; }

		const RenderCommandQueueMetricsOutput& GetCommandQueueMetrics() const { return m_CommandQueueMetricsOutput; }

		uint32_t GetCurrentImageIndex() const { return m_ImageIndex; }
		uint32_t GetCurrentFrameIndex() const { return m_CurrentFrameIndex; }
		uint32_t GetMaxFramesInFlight() const { return m_MaxFramesInFlight; }
		uint64_t GetFrameNumber() const { return m_FrameNumber; }

		virtual void OnWindowResize() = 0;
		virtual void OnViewportResize() = 0;
		virtual uint32_t OnMousePicking(const EntityPickedEvent& e, const Ref<Image>& currentFrameBufferImage) = 0;

		virtual void InitializeImGui() = 0;
	protected:
		virtual void Init() = 0;

		virtual void BeginFrame() = 0;
		virtual void RenderFrame() = 0;
		virtual void EndFrame() = 0;

		void EnqueueToRenderCommandQueue(const ExecutionBatch& batch, const std::vector<RenderSubmitFunc>& submitFuncs);

		void RecreateCommandQueue();
		void FlushCommandQueue();

		virtual void FlushDeletionQueue() = 0;

		const Ref<RenderContext>& GetRenderContext() const { return m_Context; }
		const Ref<RenderDevice>& GetRenderDevice() const { return m_RenderDevice; }
		const Ref<SwapChain>& GetSwapChain() const { return m_SwapChain; }

		RendererBackend(RendererConfiguration config, const Ref<Window>& window);

		std::vector<std::deque<RenderDeletionFunc>> m_ResourceDeletionQueues;
	protected:
		Ref<RenderContext> m_Context = nullptr;
		Ref<RenderDevice> m_RenderDevice = nullptr;
		Ref<SwapChain> m_SwapChain = nullptr;

		Ref<RenderCommandQueue> m_RenderCommandQueue = nullptr;
		RenderCommandQueueMetricsOutput m_CommandQueueMetricsOutput;

		RendererConfiguration m_RendererConfiguration;

		friend class Renderer;
		friend class RenderThread;
		friend struct ImGuiVulkanImpl;
	};
}