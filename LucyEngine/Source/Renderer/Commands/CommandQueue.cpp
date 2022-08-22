#include "lypch.h"
#include "CommandQueue.h"
#include "VulkanCommandQueue.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	Ref<CommandQueue> CommandQueue::Create() {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanCommandQueue>();
				break;
		}
		return nullptr;
	}

	RenderCommandResourceHandle CommandQueue::CreateRenderPassResource(RenderCommandFunc&& func, Ref<Pipeline> pipeline) {
		LUCY_PROFILE_NEW_EVENT("CommandQueue::CreateRenderPassResource");
		
		RenderCommandResourceHandle uniqueHandle = RenderCommandResource::CreateUniqueHandle();
		std::pair<RenderCommandResourceHandle, RenderCommandResource> pair{ uniqueHandle, RenderCommandResource(std::move(func), pipeline) };
		m_BufferMap.insert(pair);
		return uniqueHandle;
	}

	void CommandQueue::EnqueueRenderCommand(RenderCommandResourceHandle resourceHandle, const Ref<RenderCommand>& command) {
		LUCY_PROFILE_NEW_EVENT("CommandQueue::EnqueueRenderCommand");
		m_BufferMap[resourceHandle].EnqueueRenderCommand(command);
	}

	void CommandQueue::Recreate() {
		m_CommandPool->Recreate();
	}

	void CommandQueue::Free() {
		m_CommandPool->Destroy();
		Clear();
	}

	void CommandQueue::Clear() {
		m_BufferMap.clear();
	}
}