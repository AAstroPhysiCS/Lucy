#include "lypch.h"
#include "CommandQueue.h"
#include "VulkanCommandQueue.h"

#include "Renderer/Renderer.h"
#include "Renderer/Context/VulkanSwapChain.h"

namespace Lucy {

	Ref<CommandQueue> CommandQueue::Create() {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanCommandQueue>();
				break;
		}
		return nullptr;
	}

	void CommandQueue::Init() {
		CommandPoolCreateInfo createInfo;
		if (Renderer::GetCurrentRenderArchitecture() == RenderArchitecture::Vulkan) {
			const VulkanSwapChain& swapChain = VulkanSwapChain::Get();
			createInfo.CommandBufferCount = swapChain.GetMaxFramesInFlight();
			createInfo.Level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			createInfo.PoolFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		}

		m_CommandPool = CommandPool::Create(createInfo);
	}

	void CommandQueue::Enqueue(const CommandElement& element) {
		m_Buffer.push_back(element);
	}

	void CommandQueue::Recreate() {
		m_CommandPool->Recreate();
		Clear();
	}

	void CommandQueue::Free() {
		m_CommandPool->Destroy();
		Clear();
	}

	void CommandQueue::Clear() {
		m_Buffer.clear();
	}
}