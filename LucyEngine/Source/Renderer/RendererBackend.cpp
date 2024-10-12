#include "lypch.h"
#include "RendererBackend.h"
#include "Device/VulkanRenderDevice.h"

#include "Core/Application.h"

#include "RenderPass.h"
#include "Memory/Buffer/FrameBuffer.h"

#include "VulkanRenderer.h"
#include "Commands/CommandQueue.h"

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
		m_RenderContext(RenderContext::Create(config.RenderArchitecture, window)),
		m_RenderThread(Application::GetRunnableThreadScheduler()->GetThread<RenderThread>()), 
		m_CommandQueue(Memory::CreateRef<CommandQueue>(CommandQueueCreateInfo{ .CommandListParallelCount = 1, .RenderDevice = GetRenderDevice() })), 
		m_RendererConfiguration(config) {
		/* m_MaxFramesInFlight = (uint32_t)m_SwapChain.GetSwapChainImageCount(); */
	}

	void RendererBackend::Init() {
		m_RenderContext->Init();
		m_RenderContext->PrintInfo();

		m_CommandQueue->Init();
	}

	void RendererBackend::EnqueueToRenderThread(RenderCommandFunc&& func) {
		(*m_CommandQueue) += std::move(func);
	}

	void RendererBackend::EnqueueResourceDestroy(RenderResourceHandle& handle) {
		m_ResourceDeletionQueue.emplace_back([&]() {
			GetRenderDevice()->RTDestroyResource(handle);
			handle = InvalidRenderResourceHandle;
		});
	}

	void RendererBackend::EnqueueToRenderThread(RenderSubmitFunc&& func) {
		(*m_CommandQueue) += std::move(func);
	}

	void RendererBackend::SubmitToRender(RenderGraphPass& pass, RenderResourceHandle renderPassHandle, RenderResourceHandle frameBufferHandle) {
		EnqueueToRenderThread([&, renderPassHandle, frameBufferHandle](RenderCommandList& cmdList) {
			const auto& device = GetRenderDevice();
			const auto& renderPass = device->AccessResource<RenderPass>(renderPassHandle);
			const auto& frameBuffer = device->AccessResource<FrameBuffer>(frameBufferHandle);
			GetRenderDevice()->BeginRenderPass(renderPass, frameBuffer, cmdList.GetPrimaryCommandPool());
			pass.Execute(cmdList);
			GetRenderDevice()->EndRenderPass(renderPass);
		});
	}

	void RendererBackend::SubmitToCompute(RenderGraphPass& pass) {
		EnqueueToRenderThread([&](RenderCommandList& cmdList) {
			pass.Execute(cmdList);
		});
	}

	void RendererBackend::RecreateCommandQueue() {
		m_CommandQueue->Recreate();
	}

	void RendererBackend::FlushCommandQueue() {
		m_CommandQueue->FlushCommandQueue();
	}

	void RendererBackend::FlushSubmitQueue() {
		m_CommandQueueMetricsOutput.RenderTime = 0.0;
		m_CommandQueue->FlushSubmitQueue(m_CommandQueueMetricsOutput);
	}

	void RendererBackend::FlushDeletionQueue() {
		if (m_ResourceDeletionQueue.empty())
			return;
		//TODO: do vkSynchronization2 -> this way, bad for perf
		GetRenderDevice()->WaitForQueue(TargetQueueFamily::Graphics);
		GetRenderDevice()->WaitForQueue(TargetQueueFamily::Compute);
		for (const auto& deletionFunc : m_ResourceDeletionQueue)
			deletionFunc();
		m_ResourceDeletionQueue.clear();
	}

	void RendererBackend::Destroy() {
		FlushDeletionQueue();
		m_CommandQueue->Free();
		m_RenderContext->Destroy();
	}
}