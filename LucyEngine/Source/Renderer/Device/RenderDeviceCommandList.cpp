#include "lypch.h"
#include "RenderDeviceCommandList.h"
#include "VulkanRenderDeviceCommandList.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	Ref<RenderDeviceCommandList> RenderDeviceCommandList::Create() {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanRenderDeviceCommandList>();
				break;
		}
		return nullptr;
	}

	void RenderDeviceCommandList::EnqueueToRenderThread(EnqueueFunc&& func) {
		LUCY_PROFILE_NEW_EVENT("RenderDeviceCommandList::EnqueueToRenderThread");
		m_RenderFunctionQueue.push_back(std::move(func));
	}

	RenderCommandResourceHandle RenderDeviceCommandList::CreateRenderPassResource(RenderCommandFunc&& func, Ref<Pipeline> pipeline) {
		LUCY_PROFILE_NEW_EVENT("RenderDeviceCommandList::CreateRenderPassResource");
		return m_CommandQueue->CreateRenderPassResource(std::move(func), pipeline);
	}

	void RenderDeviceCommandList::DispatchCommands() {
		LUCY_PROFILE_NEW_EVENT("RenderDeviceCommandList::DispatchCommands");
		
		uint32_t oldSize = m_RenderFunctionQueue.size();
		for (uint32_t i = 0; i < oldSize; i++) {
			m_RenderFunctionQueue[i](); //functions can contain nested functions
		}
		//meaing that nested lambda functions are being run in the second iteration.
		m_RenderFunctionQueue.erase(m_RenderFunctionQueue.begin(), m_RenderFunctionQueue.begin() + oldSize);
	}

	void RenderDeviceCommandList::Free() {
		m_CommandQueue->Free();
	}

	void RenderDeviceCommandList::Recreate() {
		m_CommandQueue->Recreate();
	}
}