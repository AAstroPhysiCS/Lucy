#include "lypch.h"
#include "VulkanRenderDeviceCommandList.h"

#include "Renderer/Commands/VulkanCommandQueue.h"

namespace Lucy {

	void VulkanRenderDeviceCommandList::Init() {
		m_CommandQueue = CommandQueue::Create();
		m_CommandQueue->Init();
	}

	void VulkanRenderDeviceCommandList::ExecuteCommandQueue() {
		m_CommandQueue->Execute();
	}

	void VulkanRenderDeviceCommandList::ExecuteSingleTimeCommand(std::function<void(VkCommandBuffer)>&& func) {
		VkCommandBuffer commandBuffer = m_CommandQueue.As<VulkanCommandQueue>()->BeginSingleTimeCommand();
		func(commandBuffer);
		m_CommandQueue.As<VulkanCommandQueue>()->EndSingleTimeCommand(commandBuffer);
	}
}
