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
		m_RenderDeviceCommandList = RenderDeviceCommandList::Create();
		m_RenderDeviceCommandList->Init();
	}

	void RenderDevice::Recreate() {
		m_RenderDeviceCommandList->Recreate();
	}

	void RenderDevice::EnqueueToRenderThread(EnqueueFunc&& func) {
		m_RenderDeviceCommandList->EnqueueToRenderThread(std::forward<EnqueueFunc>(func));
	}

	RenderCommandResourceHandle RenderDevice::CreateRenderPassResource(RenderCommandFunc&& func, Ref<Pipeline> pipeline) {
		return m_RenderDeviceCommandList->CreateRenderPassResource(std::move(func), pipeline);
	}

	void RenderDevice::DispatchCommands() {
		m_RenderDeviceCommandList->DispatchCommands();
	}

	void RenderDevice::ExecuteCommandQueue() {
		m_RenderDeviceCommandList->ExecuteCommandQueue();
	}

	void RenderDevice::Destroy() {
		m_RenderDeviceCommandList->Free();
	}
}
