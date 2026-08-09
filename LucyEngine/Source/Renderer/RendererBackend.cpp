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

	void RendererBackend::EnqueueResourceDestroy(RenderDeviceResourceHandle handle) {
		auto debugName = GetRenderDevice()->AccessResource<RenderDeviceResource>(handle)->GetDebugName();

		m_ResourceDeletionQueues[GetCurrentFrameIndex()].emplace_back([=](const Ref<RenderDevice>& device) mutable {
			LUCY_INFO("Deleted Resource Name {0}", debugName);
			device->RTDestroyResource(handle);
		});
	}

	void RendererBackend::EnqueueResourceDestroy(RenderDeletionFunc&& func) {
		m_ResourceDeletionQueues[GetCurrentFrameIndex()].emplace_back(std::move(func));
	}

	void RendererBackend::EnqueueResourceRecreate(RenderRecreateFunc&& func) {
		EnqueueToRenderCommandQueue([this, func = std::move(func)](const Ref<RenderDevice>& device) mutable {
			auto deletionFunc = func(device);
			if (deletionFunc)
				EnqueueResourceDestroy(std::move(deletionFunc));
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