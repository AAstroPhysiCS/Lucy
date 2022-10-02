#include "lypch.h"

#include "VulkanCommandQueue.h"
#include "VulkanCommandPool.h"

#include "Renderer/Context/VulkanContextDevice.h"
#include "Renderer/Context/VulkanGraphicsPipeline.h"

#include "Renderer/Renderer.h"
#include "Renderer/Synchronization/VulkanSyncItems.h"

namespace Lucy {

	void VulkanCommandQueue::Init() {
		CommandPoolCreateInfo createInfo;
		createInfo.CommandBufferCount = Renderer::GetMaxFramesInFlight();
		createInfo.Level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		createInfo.PoolFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		m_CommandPool = CommandPool::Create(createInfo);
	}

	void VulkanCommandQueue::Execute() {
		LUCY_PROFILE_NEW_EVENT("VulkanCommandQueue::Execute");
		if (m_CommandResourceMap.size() == 0)
			return;

		VkCommandBuffer commandBuffer = GetCurrentCommandBuffer();

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		LUCY_VK_ASSERT(vkBeginCommandBuffer(commandBuffer, &beginInfo));

		//TODO: Multithreading here; mutex and locks for multithreading
		for (auto& [handle, resource] : m_CommandResourceMap) {
			if (resource.IsChildValid())
				continue;

			const auto& currentResourcePipeline = resource.GetTargetPipeline();

			bool isImGuiPass = currentResourcePipeline.Get() == nullptr;
			if (isImGuiPass) {
				resource.DoPass(commandBuffer);
				continue;
			}

			switch (currentResourcePipeline->GetType()) {
				case ContextPipelineType::Graphics:
					Renderer::BeginRenderPass(commandBuffer, currentResourcePipeline.As<VulkanGraphicsPipeline>());
					resource.DoPass(commandBuffer);
					for (auto& childResourceHandle : resource.m_ChildCommandResourceHandles)
						m_CommandResourceMap[childResourceHandle].DoPass(commandBuffer);
					Renderer::EndRenderPass(currentResourcePipeline.As<VulkanGraphicsPipeline>());
					break;
				case ContextPipelineType::Compute:
					resource.DoPass(commandBuffer);
					break;
				default:
					LUCY_ASSERT(false);
			}
		}

		LUCY_VK_ASSERT(vkEndCommandBuffer(commandBuffer));
	}

	void VulkanCommandQueue::SubmitWorkToGPU(void* queueHandle, const Fence& currentFrameFence, const Semaphore& currentFrameWaitSemaphore, const Semaphore& currentFrameSignalSemaphore) const {
		LUCY_PROFILE_NEW_EVENT("VulkanCommandQueue::SubmitToQueue");

		VkCommandBuffer currentCommandBuffer = GetCurrentCommandBuffer();

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkPipelineStageFlags imageWaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &currentFrameWaitSemaphore.GetSemaphore();
		submitInfo.pWaitDstStageMask = imageWaitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &currentCommandBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &currentFrameSignalSemaphore.GetSemaphore();

		LUCY_VK_ASSERT(vkQueueSubmit((VkQueue)queueHandle, 1, &submitInfo, currentFrameFence.GetFence()));
	}

	void VulkanCommandQueue::SubmitWorkToGPU(void* queueHandle, uint32_t commandBufferCount, void* commandBufferHandles) const {
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = commandBufferCount;
		submitInfo.pCommandBuffers = (VkCommandBuffer*)&commandBufferHandles;

		vkQueueSubmit((VkQueue)queueHandle, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle((VkQueue)queueHandle);
	}

	VkCommandBuffer VulkanCommandQueue::BeginSingleTimeCommand() const {
		return m_CommandPool.As<VulkanCommandPool>()->BeginSingleTimeCommand();
	}

	void VulkanCommandQueue::EndSingleTimeCommand() const {
		const auto& vulkanCommandPool = m_CommandPool.As<VulkanCommandPool>();
		vulkanCommandPool->EndSingleTimeCommand();

		VkQueue targetedQueue = VulkanContextDevice::Get().GetGraphicsQueue();
		SubmitWorkToGPU(targetedQueue, 1, vulkanCommandPool->GetCommandBuffer(vulkanCommandPool->GetCommandBufferSize() - 1));

		vulkanCommandPool->FreeCommandBuffers(1, vulkanCommandPool->GetCommandBufferSize() - 1);
	}

	VkCommandBuffer VulkanCommandQueue::GetCurrentCommandBuffer() const {
		return m_CommandPool.As<VulkanCommandPool>()->GetCommandBuffer(Renderer::GetCurrentFrameIndex());
	}
}