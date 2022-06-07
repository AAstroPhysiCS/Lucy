#include "lypch.h"
#include "CommandQueue.h"

#include "VulkanCommandPool.h"
#include "Context/VulkanSwapChain.h"
#include "Context/VulkanDevice.h"
#include "DrawCommand.h"

namespace Lucy {

	void CommandQueue::Init() {
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();

		CommandPoolSpecs specs;
		specs.CommandBufferCount = swapChain.GetMaxFramesInFlight();
		specs.Level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		specs.PoolFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		m_CommandPool = VulkanCommandPool::Create(specs);
	}

	void CommandQueue::Enqueue(const CommandElement& element) {
		m_Buffer.push_back(element);
	}

	VkCommandBuffer CommandQueue::BeginSingleTimeCommand() const {
		return m_CommandPool->BeginSingleTimeCommand();
	}

	void CommandQueue::EndSingleTimeCommand(VkCommandBuffer commandBuffer) const {
		m_CommandPool->EndSingleTimeCommand(commandBuffer);
	}

	void CommandQueue::Execute() {
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		VkResult result = swapChain.GetLastSwapChainResult();
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
			return;

		VkCommandBuffer commandBuffer = GetCurrentCommandBuffer();

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		LUCY_VK_ASSERT(vkBeginCommandBuffer(commandBuffer, &beginInfo));

		//TODO: Multithreading here; mutex and locks for multithreading
		for (uint32_t i = 0; i < m_Buffer.size(); i++) {
			CommandElement& element = m_Buffer[i];
			element.RecordFunc(element.Argument);
		}

		LUCY_VK_ASSERT(vkEndCommandBuffer(commandBuffer));
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

	VkCommandBuffer CommandQueue::GetCurrentCommandBuffer() const {
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		return m_CommandPool->GetCommandBuffer(swapChain.GetCurrentFrameIndex());
	}
}