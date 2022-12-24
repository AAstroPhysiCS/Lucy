#include "lypch.h"

#include "VulkanSyncItems.h"
#include "Renderer/Context/VulkanContextDevice.h"

#include "Renderer/Image/VulkanImage2D.h"

namespace Lucy {

	Semaphore::Semaphore() {
		VkSemaphoreCreateInfo createInfo = VulkanAPI::SemaphoreCreateInfo();
		LUCY_VK_ASSERT(vkCreateSemaphore(VulkanContextDevice::Get().GetLogicalDevice(), &createInfo, nullptr, &m_Handle));
	}

	void Semaphore::Destroy() {
		vkDestroySemaphore(VulkanContextDevice::Get().GetLogicalDevice(), m_Handle, nullptr);
	}

	Fence::Fence() {
		VkFenceCreateInfo createInfo = VulkanAPI::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
		LUCY_VK_ASSERT(vkCreateFence(VulkanContextDevice::Get().GetLogicalDevice(), &createInfo, nullptr, &m_Handle));
	}

	void Fence::Destroy() {
		vkDestroyFence(VulkanContextDevice::Get().GetLogicalDevice(), m_Handle, nullptr);
	}

	ImageMemoryBarrier::ImageMemoryBarrier(const ImageMemoryBarrierCreateInfo& createInfo)
		: m_CreateInfo(createInfo), 
		m_Barrier(VulkanAPI::ImageMemoryBarrier(m_CreateInfo.ImageHandle, m_CreateInfo.OldLayout, m_CreateInfo.NewLayout, m_CreateInfo.SubResourceRange)) {
	}

	void ImageMemoryBarrier::RunBarrier(VkCommandBuffer commandBuffer) {
		VkPipelineStageFlags sourceStage, destStage;
		DefineMasksByLayout(m_CreateInfo.OldLayout, m_CreateInfo.NewLayout, m_Barrier.srcAccessMask, m_Barrier.dstAccessMask, sourceStage, destStage);

		vkCmdPipelineBarrier(commandBuffer, sourceStage, destStage, 0, 0, nullptr, 0, nullptr, 1, &m_Barrier);
	}

	void DefineMasksByLayout(VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags& srcAccessMask,
												 VkAccessFlags& destAccessMask, VkPipelineStageFlags& sourceStage, VkPipelineStageFlags& destStage) {
		switch (oldLayout) {
			case VK_IMAGE_LAYOUT_UNDEFINED:
				srcAccessMask = 0;
				sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				break;
			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				break;
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				break;
			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				break;
			case VK_IMAGE_LAYOUT_GENERAL:
				srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
				sourceStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
				break;
			default:
				LUCY_ASSERT(false);
		}

		switch (newLayout) {
			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				destAccessMask = VK_ACCESS_SHADER_READ_BIT;
				destStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				break;
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				destAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				destStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				break;
			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				destAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				destStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				break;
			case VK_IMAGE_LAYOUT_GENERAL:
				destAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
				destStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
				break;
			default:
				LUCY_ASSERT(false);
		}
	}
}