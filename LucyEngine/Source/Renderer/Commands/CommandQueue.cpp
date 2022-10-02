#include "lypch.h"
#include "CommandQueue.h"
#include "VulkanCommandQueue.h"

#include "Renderer/Context/GraphicsPipeline.h"

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

	CommandResourceHandle CommandQueue::CreateCommandResource(Ref<ContextPipeline> pipeline, CommandFunc&& func) {
		LUCY_PROFILE_NEW_EVENT("CommandQueue::CreateCommandResource");
		CommandResourceHandle uniqueHandle = CommandResource::CreateUniqueHandle();
		CommandResource commandResource = CommandResource(pipeline, std::move(func));
		m_CommandResourceMap.emplace(uniqueHandle, commandResource);

		return uniqueHandle;
	}

	CommandResourceHandle CommandQueue::CreateChildCommandResource(CommandResourceHandle parentResourceHandle, Ref<GraphicsPipeline> childPipeline, CommandFunc&& func) {
		LUCY_PROFILE_NEW_EVENT("CommandQueue::CreateChildCommandResource");
		CommandResource& parentCommandResource = m_CommandResourceMap[parentResourceHandle];

		const Ref<GraphicsPipeline>& parentPipeline = parentCommandResource.GetTargetPipeline().As<GraphicsPipeline>();
		if (parentPipeline->GetRenderPass() != childPipeline->GetRenderPass() || parentPipeline->GetFrameBuffer() != childPipeline->GetFrameBuffer()) {
			LUCY_CRITICAL("This configuration cannot be processed!");
			LUCY_CRITICAL("Child pipeline must have the same renderpass and framebuffer as the parent pipeline.");
			LUCY_ASSERT(false);
		}

		CommandResourceHandle childUniqueHandle = CommandResource::CreateUniqueHandle();
		CommandResource childCommandResource = CommandResource(childPipeline, std::move(func), true);
		parentCommandResource.m_ChildCommandResourceHandles.push_back(childUniqueHandle);

		m_CommandResourceMap.emplace(childUniqueHandle, childCommandResource);

		return childUniqueHandle;
	}

	void CommandQueue::DeleteCommandResource(CommandResourceHandle commandHandle) {
		if (m_CommandResourceMap.find(commandHandle) == m_CommandResourceMap.end()) {
			LUCY_CRITICAL("Could not find a handle for a given command resource");
			LUCY_ASSERT(false);
			return;
		}
		m_CommandResourceMap.erase(commandHandle);
	}

	void CommandQueue::EnqueueCommand(CommandResourceHandle resourceHandle, const Ref<RenderCommand>& command) {
		LUCY_PROFILE_NEW_EVENT("CommandQueue::EnqueueCommand");
		m_CommandResourceMap[resourceHandle].EnqueueCommand(command);
	}

	void CommandQueue::Recreate() {
		m_CommandPool->Recreate();
	}

	void CommandQueue::Free() {
		m_CommandPool->Destroy();
		Clear();
	}

	void CommandQueue::Clear() {
		m_CommandResourceMap.clear();
	}
}