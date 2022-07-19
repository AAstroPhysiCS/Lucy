#include "lypch.h"
#include "CommandQueue.h"

#include "VulkanCommandPool.h"

#include "Context/VulkanSwapChain.h"
#include "Context/VulkanDevice.h"
#include "Context/VulkanPipeline.h"

#include "Renderer.h"

namespace Lucy {

	void CommandQueue::Init() {
		const VulkanSwapChain& swapChain = VulkanSwapChain::Get();

		CommandPoolCreateInfo createInfo;
		createInfo.CommandBufferCount = swapChain.GetMaxFramesInFlight();
		createInfo.Level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		createInfo.PoolFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		m_CommandPool = VulkanCommandPool::Create(createInfo);
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

		if (m_Buffer.size() == 0)
			return;

		VkCommandBuffer commandBuffer = GetCurrentCommandBuffer();

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		LUCY_VK_ASSERT(vkBeginCommandBuffer(commandBuffer, &beginInfo));

		//TODO: Multithreading here; mutex and locks for multithreading
		for (uint32_t i = 0; i < m_Buffer.size(); i++) {
			CommandElement& element = m_Buffer[i];
			std::vector<Ref<DrawCommand>>& drawCommandArguments = element.Arguments;
			Ref<Pipeline>& pipeline = element.Pipeline;

			if (pipeline) {
				//TODO: DELETE THIS
				Renderer::UpdateResources(drawCommandArguments, pipeline);

				Renderer::BeginRenderPass(pipeline, commandBuffer);
				
				Renderer::BindPipeline(pipeline);
				Renderer::BindDescriptorSet(pipeline, 0);
				Renderer::BindDescriptorSet(pipeline, 1);

				for (Ref<DrawCommand>& drawCommand : drawCommandArguments)
					element.RecordFunc(drawCommand);

				Renderer::EndRenderPass(pipeline);
			} else {
				element.RecordFunc(nullptr); //for imgui
			}
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