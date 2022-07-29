#include "lypch.h"
#include "VulkanCommandQueue.h"
#include "VulkanCommandPool.h"

#include "Renderer/Context/VulkanSwapChain.h"
#include "Renderer/Renderer.h"

namespace Lucy {

	void VulkanCommandQueue::Init() {
		CommandPoolCreateInfo createInfo;
		createInfo.CommandBufferCount = VulkanSwapChain::Get().GetMaxFramesInFlight();
		createInfo.Level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		createInfo.PoolFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		m_CommandPool = CommandPool::Create(createInfo);
	}

	void VulkanCommandQueue::Execute() {
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		VkResult result = swapChain.GetLastSwapChainResult();
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
			return;

		if (m_BufferMap.size() == 0)
			return;

		VkCommandBuffer commandBuffer = GetCurrentCommandBuffer();

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		LUCY_VK_ASSERT(vkBeginCommandBuffer(commandBuffer, &beginInfo));

		//TODO: Multithreading here; mutex and locks for multithreading
		for (auto& [handle, resource] : m_BufferMap) {
			const auto& targetPipeline = resource.GetTargetPipeline();

			if (targetPipeline) {
				Renderer::BeginRenderPass(commandBuffer, targetPipeline);
				resource.DoPass(commandBuffer);
				Renderer::EndRenderPass(targetPipeline);
			} else { //for imgui
				resource.DoPass(commandBuffer);
			}
		}

		LUCY_VK_ASSERT(vkEndCommandBuffer(commandBuffer));
	}

	VkCommandBuffer VulkanCommandQueue::BeginSingleTimeCommand() const {
		return m_CommandPool.As<VulkanCommandPool>()->BeginSingleTimeCommand();
	}

	void VulkanCommandQueue::EndSingleTimeCommand(VkCommandBuffer commandBuffer) const {
		m_CommandPool.As<VulkanCommandPool>()->EndSingleTimeCommand(commandBuffer);
	}

	VkCommandBuffer VulkanCommandQueue::GetCurrentCommandBuffer() const {
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		return m_CommandPool.As<VulkanCommandPool>()->GetCommandBuffer(swapChain.GetCurrentFrameIndex());
	}
}