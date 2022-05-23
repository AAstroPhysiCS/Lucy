#include "lypch.h"
#include "CommandQueue.h"

#include "VulkanCommandPool.h"
#include "Context/VulkanSwapChain.h"
#include "Renderer/DrawCommand.h"

namespace Lucy {

	/*
	void CommandQueue::Grow(const CommandElement& element) const {
		//copying the old data and deleting the old data
		uint32_t newSize = m_Size + 1;
		CommandElement** newBuffer = new CommandElement * [newSize];
		memcpy(newBuffer, m_Buffer, m_Size * sizeof(CommandElement));

		delete[] m_Buffer;
		//inserting new data to the newly created buffer
		memcpy(newBuffer[m_Size + 1], &element, sizeof(CommandElement));

		m_Size = newSize;
		m_Buffer = newBuffer;
	}
	*/

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
		
		/*
		if (!m_Buffer && m_Size == 0) {
			m_Size++;
			m_Buffer = new CommandElement*[m_Size];
			memcpy(m_Buffer[0], &element, sizeof(CommandElement));
			return;
		}
		*/

		//Grow(element);
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

		VkViewport viewport;
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = swapChain.GetExtent().width;
		viewport.height = swapChain.GetExtent().height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swapChain.GetExtent();

		if (viewport.width == 0 || viewport.height == 0) 
			return;

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		LUCY_VK_ASSERT(vkBeginCommandBuffer(commandBuffer, &beginInfo));

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		//TODO: Multithreading here; mutex and locks for multithreading
		for (uint32_t i = 0; i < m_Buffer.size(); i++) {
			const CommandElement& element = m_Buffer[i];
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