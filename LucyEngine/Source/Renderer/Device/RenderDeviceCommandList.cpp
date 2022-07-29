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
		m_RenderFunctionQueue.push_back(std::move(func));
	}

	RenderCommandResourceHandle RenderDeviceCommandList::CreateRenderPassResource(RenderCommandFunc&& func, Ref<Pipeline> pipeline) {
		return m_CommandQueue->CreateRenderPassResource(std::move(func), pipeline);
	}

	void RenderDeviceCommandList::DispatchCommands() {
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