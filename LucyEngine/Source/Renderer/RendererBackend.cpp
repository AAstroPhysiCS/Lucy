#include "lypch.h"
#include "RendererBackend.h"

#include "Core/Application.h"

#include "Device/RenderDevice.h"

#include "RenderPass.h"
#include "Memory/Buffer/FrameBuffer.h"

#include "VulkanRenderer.h"
#include "Commands/RenderCommandQueue.h"

namespace Lucy {

	Ref<RendererBackend> RendererBackend::Create(RendererConfiguration config, const Ref<Window>& window) {
		switch (config.RenderArchitecture) {
			case RenderArchitecture::Vulkan: {
				return Memory::CreateRef<VulkanRenderer>(config, window);
				break;
			}
			default:
				LUCY_ASSERT(false, "No suitable API found to create the renderer!");
				break;
		}
		return nullptr;
	}

	RendererBackend::RendererBackend(RendererConfiguration config, const Ref<Window>& window)
		: m_MaxFramesInFlight(3),
		m_Context(RenderContext::Create(config.RenderArchitecture, window)),
		m_RenderDevice(RenderDevice::Create(config)),
		m_RenderCommandQueue(Memory::CreateRef<RenderCommandQueue>(RenderCommandQueueCreateInfo{ .RenderDevice = m_RenderDevice, .MaxFramesInFlight = m_MaxFramesInFlight })),
		m_SwapChain(SwapChain::Create(config.RenderArchitecture, window, m_RenderDevice)),
		m_RendererConfiguration(config) {
		/* m_MaxFramesInFlight = (uint32_t)m_SwapChain.GetSwapChainImageCount(); */
		m_ResourceDeletionQueues.resize(m_MaxFramesInFlight);
	}

	void RendererBackend::EnqueueToRenderCommandQueue(RenderCommandFunc&& func) {
		(*m_RenderCommandQueue) += std::move(func);
	}

	void RendererBackend::EnqueueToRenderCommandQueue(const ExecutionBatch& batch, const std::vector<RenderSubmitFunc>& submitFuncs) {
		(*m_RenderCommandQueue) += RenderSubmitInfo{ .Batch = batch, .SubmitFuncs = submitFuncs };
	}

	void RendererBackend::EnqueueResourceDestroy(RenderResourceHandle handle) {
		m_ResourceDeletionQueues[GetCurrentFrameIndex()].emplace_back([&, handle]() mutable {
			LUCY_INFO("Debug Name {0}, ", GetRenderDevice()->AccessResource<RenderResource>(handle)->GetDebugName());
			GetRenderDevice()->RTDestroyResource(handle);
		});
	}

	void RendererBackend::RecreateCommandQueue() {
		m_RenderCommandQueue->Recreate();
	}

	void RendererBackend::FlushCommandQueue() {
		m_RenderCommandQueue->FlushCommandQueue();
	}

	void RendererBackend::Destroy() {
		m_RenderCommandQueue->Destroy();
		m_RenderDevice->Destroy();
	}
}