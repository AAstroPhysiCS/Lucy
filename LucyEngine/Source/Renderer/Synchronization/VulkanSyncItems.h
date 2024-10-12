#pragma once

#include "vulkan/vulkan.h"

namespace Lucy {

	class VulkanImage2D;

	//TODO: VK Synchronization2
	class Semaphore final {
	public:
		Semaphore();
		~Semaphore() = default;

		void Destroy();

		inline const VkSemaphore& GetSemaphore() const { return m_Handle; }
	private:
		VkSemaphore m_Handle = VK_NULL_HANDLE;
	};

	class Fence final {
	public:
		Fence();
		~Fence() = default;

		void Destroy();

		inline const VkFence& GetFence() const { return m_Handle; }
	private:
		VkFence m_Handle = VK_NULL_HANDLE;
	};

	struct ImageMemoryBarrierCreateInfo {
		VkImage ImageHandle = VK_NULL_HANDLE;

		//abstracted this, since we need this when we do mipmapping
		VkImageSubresourceRange SubResourceRange;

		VkImageLayout OldLayout;
		VkImageLayout NewLayout;
	};

	extern void DefineMasksByLayout(VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags& srcAccessMask, VkAccessFlags& destAccessMask,
							 VkPipelineStageFlags& sourceStage, VkPipelineStageFlags& destStage);

	//for image layout transitions
	class ImageMemoryBarrier final {
	public:
		ImageMemoryBarrier(const ImageMemoryBarrierCreateInfo& createInfo);
		~ImageMemoryBarrier() = default;

		void RunBarrier(VkCommandBuffer commandBuffer);
	private:
		ImageMemoryBarrierCreateInfo m_CreateInfo;
		VkImageMemoryBarrier m_Barrier{};
	};
}