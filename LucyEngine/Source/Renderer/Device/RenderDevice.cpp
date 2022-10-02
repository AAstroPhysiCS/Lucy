#include "lypch.h"
#include "RenderDevice.h"
#include "VulkanRenderDevice.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	Ref<RenderDevice> RenderDevice::Create() {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanRenderDevice>();
				break;
		}
		return nullptr;
	}

	void RenderDevice::Init() {
		m_CommandQueue = CommandQueue::Create();
		m_CommandQueue->Init();
	}

	void RenderDevice::Recreate() {
		m_CommandQueue->Recreate();
	}

	void RenderDevice::EnqueueToRenderThread(EnqueueFunc&& func) {
		LUCY_PROFILE_NEW_EVENT("RenderDevice::EnqueueToRenderThread");
		m_RenderFunctionQueue.push_back(std::move(func));
	}

	void RenderDevice::SubmitWorkToGPU(void* queueHandle, const Fence& currentFrameFence, const Semaphore& currentFrameWaitSemaphore, const Semaphore& currentFrameSignalSemaphore) {
		LUCY_PROFILE_NEW_EVENT("RenderDevice::SubmitToQueue");
		m_CommandQueue->SubmitWorkToGPU(queueHandle, currentFrameFence, currentFrameWaitSemaphore, currentFrameSignalSemaphore);
	}

	CommandResourceHandle RenderDevice::CreateCommandResource(Ref<ContextPipeline> pipeline, CommandFunc&& func) {
		LUCY_PROFILE_NEW_EVENT("RenderDevice::CreateCommandResource");
		return m_CommandQueue->CreateCommandResource(pipeline, std::move(func));
	}

	CommandResourceHandle RenderDevice::CreateChildCommandResource(CommandResourceHandle parentResourceHandle, Ref<GraphicsPipeline> childPipeline, CommandFunc&& func) {
		LUCY_PROFILE_NEW_EVENT("RenderDevice::CreateChildCommandResource");
		return m_CommandQueue->CreateChildCommandResource(parentResourceHandle, childPipeline, std::move(func));
	}

	void RenderDevice::EnqueueCommandResourceFree(CommandResourceHandle resourceHandle) {
		LUCY_PROFILE_NEW_EVENT("RenderDevice::EnqueueCommandResourceFree");
		m_DeletionQueue.push_back([=]() {
			m_CommandQueue->DeleteCommandResource(resourceHandle);
		});
	}
	
	void RenderDevice::EnqueueResourceFree(EnqueueFunc&& func) {
		LUCY_PROFILE_NEW_EVENT("RenderDevice::EnqueueCommandResourceFree");
		m_DeletionQueue.push_back(func);
	}

	void RenderDevice::DispatchCommands() {
		LUCY_PROFILE_NEW_EVENT("RenderDevice::DispatchCommands");

		uint32_t oldSize = m_RenderFunctionQueue.size();
		for (uint32_t i = 0; i < oldSize; i++) {
			m_RenderFunctionQueue[i](); //functions can contain nested functions
		}
		//meaing that nested lambda functions are being run in the second iteration.
		m_RenderFunctionQueue.erase(m_RenderFunctionQueue.begin(), m_RenderFunctionQueue.begin() + oldSize);
	}

	void RenderDevice::ExecuteCommandQueue() {
		LUCY_PROFILE_NEW_EVENT("RenderDevice::ExecuteCommandQueue");
		m_CommandQueue->Execute();

		uint32_t oldSizeDeletionQueue = m_DeletionQueue.size();
		for (uint32_t i = 0; i < oldSizeDeletionQueue; i++) {
			m_DeletionQueue[i]();
		}
		m_DeletionQueue.erase(m_DeletionQueue.begin(), m_DeletionQueue.begin() + oldSizeDeletionQueue);
	}

	void RenderDevice::Destroy() {
		m_CommandQueue->Free();
	}
}
