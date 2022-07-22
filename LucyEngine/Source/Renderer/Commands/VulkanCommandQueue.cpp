#include "lypch.h"
#include "VulkanCommandQueue.h"
#include "VulkanCommandPool.h"

#include "Renderer/Renderer.h"
#include "Renderer/Context/VulkanSwapChain.h"
#include "Renderer/Context/Pipeline.h"
#include "Renderer/Descriptors/VulkanDescriptorSet.h"

namespace Lucy {

	void VulkanCommandQueue::Execute() {
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		VkResult result = swapChain.GetLastSwapChainResult();
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
			return;

		if (m_Buffer.size() == 0)
			return;

		VkCommandBuffer commandBuffer = GetCurrentCommandBuffer();

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		LUCY_VK_ASSERT(vkBeginCommandBuffer(commandBuffer, &beginInfo));

		//TODO: Multithreading here; mutex and locks for multithreading
		for (uint32_t i = 0; i < m_Buffer.size(); i++) {
			CommandElement& element = m_Buffer[i];
			std::vector<Ref<DrawCommand>>& drawCommandArguments = element.Arguments;
			Ref<Pipeline>& pipeline = element.Pipeline;

			if (pipeline) {
				Renderer::UpdateResources(drawCommandArguments, pipeline);

				Renderer::BeginRenderPass(commandBuffer, pipeline);

				Renderer::BindPipeline(commandBuffer, pipeline);

				for (uint32_t i = 0; i < pipeline->GetDescriptorSets().size(); i++) {
					const Ref<VulkanDescriptorSet>& descriptorSet = pipeline->GetDescriptorSets()[i].As<VulkanDescriptorSet>();
					Renderer::BindDescriptorSet(commandBuffer, pipeline, descriptorSet);
				}

				for (Ref<DrawCommand>& drawCommand : drawCommandArguments)
					element.RecordFunc(commandBuffer, drawCommand);

				Renderer::EndRenderPass(pipeline);
			} else {
				element.RecordFunc(commandBuffer, nullptr); //for imgui
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