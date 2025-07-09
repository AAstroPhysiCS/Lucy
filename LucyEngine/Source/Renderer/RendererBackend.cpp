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
		m_RenderCommandQueue(Memory::CreateRef<RenderCommandQueue>(RenderCommandQueueCreateInfo{ .CommandListParallelCount = 1, .RenderDevice = m_RenderDevice, .TargetQueueFamily = TargetQueueFamily::Graphics })),
		m_RenderComputeCommandQueue(Memory::CreateRef<RenderCommandQueue>(RenderCommandQueueCreateInfo{ .CommandListParallelCount = 1, .RenderDevice = m_RenderDevice, .TargetQueueFamily = TargetQueueFamily::Compute })),
		m_SwapChain(SwapChain::Create(config.RenderArchitecture, window, m_RenderDevice)),
		m_RendererConfiguration(config) {
		/* m_MaxFramesInFlight = (uint32_t)m_SwapChain.GetSwapChainImageCount(); */
		m_ResourceDeletionQueues.resize(m_MaxFramesInFlight);
	}

	void RendererBackend::EnqueueToRenderCommandQueue(RenderCommandFunc&& func) {
		(*m_RenderCommandQueue) += std::move(func);
	}

	void RendererBackend::EnqueueToRenderCommandQueue(RenderSubmitFunc&& func) {
		(*m_RenderCommandQueue) += std::move(func);
	}

	void RendererBackend::EnqueueResourceDestroy(RenderResourceHandle handle) {
		m_ResourceDeletionQueues[GetCurrentFrameIndex()].emplace_back([&, handle]() mutable {
			LUCY_INFO("Debug Name {0}, ", GetRenderDevice()->AccessResource<RenderResource>(handle)->GetDebugName());
			GetRenderDevice()->RTDestroyResource(handle);
		});
	}

	void RendererBackend::SubmitToRender(RenderGraphPass& pass, RenderResourceHandle renderPassHandle, RenderResourceHandle frameBufferHandle) {
		//LUCY_ASSERT(!Renderer::IsOnRenderThread(), "SubmitToRender should only be called on the main thread!");
		EnqueueToRenderCommandQueue([&, renderPassHandle, frameBufferHandle](RenderCommandList& cmdList) {
			LUCY_PROFILE_NEW_EVENT("RendererBackend::SubmitToRender");
			const auto& device = GetRenderDevice();
			const auto& renderPass = device->AccessResource<RenderPass>(renderPassHandle);
			const auto& frameBuffer = device->AccessResource<FrameBuffer>(frameBufferHandle);
			device->BeginRenderPass(renderPass, frameBuffer, cmdList.GetPrimaryCommandPool());
			pass.Execute(cmdList);
			device->EndRenderPass(renderPass);
		});
	}

	void RendererBackend::SubmitToCompute(RenderGraphPass& pass) {
		(*m_RenderComputeCommandQueue) += ([&](RenderCommandList& cmdList) {
			LUCY_PROFILE_NEW_EVENT("RendererBackend::SubmitToCompute");
			pass.Execute(cmdList);
		});
	}

	void RendererBackend::RecreateCommandQueue() {
		m_RenderCommandQueue->Recreate();
	}

	void RendererBackend::FlushCommandQueue() {
		m_RenderCommandQueue->FlushCommandQueue();
		m_RenderComputeCommandQueue->FlushCommandQueue();
	}

	void RendererBackend::FlushSubmitQueue() {
		m_CommandQueueMetricsOutput.RenderTime = 0.0;
		m_RenderCommandQueue->FlushSubmitQueue(m_CommandQueueMetricsOutput);
		m_RenderComputeCommandQueue->FlushSubmitQueue(m_CommandQueueMetricsOutputCompute);
	}

	void RendererBackend::Destroy() {
		m_RenderCommandQueue->Free();
		m_RenderDevice->Destroy();
	}
}