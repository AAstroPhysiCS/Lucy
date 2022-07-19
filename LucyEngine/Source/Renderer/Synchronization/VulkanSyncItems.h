#pragma once

#include "vulkan/vulkan.h"

namespace Lucy {

	class VulkanImage2D;

	//TODO: VK Synchronization2
	class Semaphore {
	public:
		Semaphore();
		~Semaphore() = default;

		void Destroy();

		inline VkSemaphore& GetSemaphore() { return m_Handle; }
	private:
		VkSemaphore m_Handle;
	};

	class Fence {
	public:
		Fence();
		~Fence() = default;

		void Destroy();

		inline VkFence& GetFence() { return m_Handle; }
	private:
		VkFence m_Handle;
	};

	struct ImageMemoryBarrierCreateInfo {
		VkImage ImageHandle;

		//abstracted this, since we need this when we do mipmapping
		VkImageSubresourceRange SubResourceRange;

		VkImageLayout OldLayout;
		VkImageLayout NewLayout;
	};

	//for image layout transitions
	class ImageMemoryBarrier {
	public:
		ImageMemoryBarrier(const ImageMemoryBarrierCreateInfo& createInfo);
		~ImageMemoryBarrier() = default;

		void RunBarrier(VkCommandBuffer commandBuffer);
	private:
		void DefineMasksByLayout(VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags& srcAccessMask, VkAccessFlags& destAccessMask,
								 VkPipelineStageFlags& sourceStage, VkPipelineStageFlags& destStage);

		ImageMemoryBarrierCreateInfo m_CreateInfo;
		VkImageMemoryBarrier m_Barrier{};
	};
}