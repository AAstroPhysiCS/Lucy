#include "lypch.h"

#include "VulkanCommandQueue.h"
#include "VulkanCommandPool.h"

#include "Renderer/Context/VulkanContextDevice.h"
#include "Renderer/Context/VulkanGraphicsPipeline.h"
#include "Renderer/Device/VulkanRenderDevice.h"

#include "Renderer/Renderer.h"

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
		if (m_CommandResourceMap.empty())
			return;

		VkCommandBuffer commandBuffer = GetCurrentCommandBuffer();

		VkCommandBufferBeginInfo beginInfo = VulkanAPI::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		LUCY_VK_ASSERT(vkBeginCommandBuffer(commandBuffer, &beginInfo));

		Renderer::BeginRenderDeviceTimestamp(commandBuffer);

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
					for (const auto& childResourceHandle : resource.m_ChildCommandResourceHandles)
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

		Renderer::EndRenderDeviceTimestamp(commandBuffer);

		LUCY_VK_ASSERT(vkEndCommandBuffer(commandBuffer));
	}

	void VulkanCommandQueue::SubmitWorkToGPU(void* queueHandle, const Fence& currentFrameFence, const Semaphore& currentFrameWaitSemaphore, const Semaphore& currentFrameSignalSemaphore) const {
		LUCY_PROFILE_NEW_EVENT("VulkanCommandQueue::SubmitToQueue");

		VkCommandBuffer currentCommandBuffer = GetCurrentCommandBuffer();
		VkPipelineStageFlags imageWaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		VkSubmitInfo submitInfo = VulkanAPI::QueueSubmitInfo(1, &currentCommandBuffer, 1, &currentFrameWaitSemaphore.GetSemaphore(), imageWaitStages, 1, &currentFrameSignalSemaphore.GetSemaphore());
		LUCY_VK_ASSERT(vkQueueSubmit((VkQueue)queueHandle, 1, &submitInfo, currentFrameFence.GetFence()));
	}

	void VulkanCommandQueue::SubmitWorkToGPU(void* queueHandle, uint32_t commandBufferCount, void* commandBufferHandles) const {
		LUCY_PROFILE_NEW_EVENT("VulkanCommandQueue::SubmitToQueue");
		
		VkFence fenceHandle = m_ImmediateCommandFence->GetFence();
		vkResetFences(VulkanContextDevice::Get().GetLogicalDevice(), 1, &fenceHandle);

		VkSubmitInfo submitInfo = VulkanAPI::QueueSubmitInfo(commandBufferCount, (VkCommandBuffer*)&commandBufferHandles, 0, nullptr, nullptr, 0, nullptr);
		vkQueueSubmit((VkQueue)queueHandle, 1, &submitInfo, fenceHandle);
		vkWaitForFences(VulkanContextDevice::Get().GetLogicalDevice(), 1, &fenceHandle, VK_TRUE, UINT64_MAX);
	}

	void VulkanCommandQueue::Free() {
		CommandQueue::Free();
		m_ImmediateCommandFence->Destroy();
	}

	VkCommandBuffer VulkanCommandQueue::BeginSingleTimeCommand() const {
		return m_CommandPool.As<VulkanCommandPool>()->BeginSingleTimeCommand();
	}

	void VulkanCommandQueue::EndSingleTimeCommand() const {
		const auto& vulkanCommandPool = m_CommandPool.As<VulkanCommandPool>();
		vulkanCommandPool->EndSingleTimeCommand();

		VkQueue targetedQueue = VulkanContextDevice::Get().GetGraphicsQueue();
		SubmitWorkToGPU(targetedQueue, 1u, vulkanCommandPool->GetCommandBuffer(vulkanCommandPool->GetCommandBufferSize() - 1));

		vulkanCommandPool->FreeCommandBuffers(1u, vulkanCommandPool->GetCommandBufferSize() - 1);
	}

	VkCommandBuffer VulkanCommandQueue::GetCurrentCommandBuffer() const {
		return m_CommandPool.As<VulkanCommandPool>()->GetCommandBuffer(Renderer::GetCurrentFrameIndex());
	}
}