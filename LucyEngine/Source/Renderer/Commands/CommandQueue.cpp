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

	CommandResourceHandle CommandQueue::CreateCommandResource(CommandFunc&& func, Ref<GraphicsPipeline> pipeline) {
		LUCY_PROFILE_NEW_EVENT("CommandQueue::CreateCommandResource");
		
		CommandResourceHandle uniqueHandle = RenderCommandResource::CreateUniqueHandle();
		std::pair<CommandResourceHandle, RenderCommandResource> pair{ uniqueHandle, RenderCommandResource(std::move(func), pipeline) };
		m_RenderResourceMap.insert(pair);
		return uniqueHandle;
	}

	void CommandQueue::DeleteCommandResource(CommandResourceHandle commandHandle) {
		if (m_RenderResourceMap.find(commandHandle) == m_RenderResourceMap.end()) {
			LUCY_CRITICAL("Could not find a handle for a given command resource");
			LUCY_ASSERT(false);
			return;
		}
		m_RenderResourceMap.erase(commandHandle);
	}

	void CommandQueue::EnqueueCommand(CommandResourceHandle resourceHandle, const Ref<RenderCommand>& command) {
		LUCY_PROFILE_NEW_EVENT("CommandQueue::EnqueueCommand");
		m_RenderResourceMap[resourceHandle].EnqueueCommand(command);
	}

	void CommandQueue::Recreate() {
		m_CommandPool->Recreate();
	}

	void CommandQueue::Free() {
		m_CommandPool->Destroy();
		Clear();
	}

	void CommandQueue::Clear() {
		m_RenderResourceMap.clear();
	}
}