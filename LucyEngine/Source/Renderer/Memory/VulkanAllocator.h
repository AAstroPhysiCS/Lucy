#pragma once

#include "vma/vk_mem_alloc.h"

#include "VulkanRenderDeviceUploadManager.h"

namespace Lucy {

	enum class MemoryUsage {
		/*
		* Auto, with no additional flag
		*/
		Auto,

		/*
		* A buffer that u can map with vmaMapMemory.
		* Resides on the CPU Ram
		*/
		CPUOnly,

		CPUToGPU,
		
		/*
		* Any resources that you frequently write and read on GPU, e.g. images used as color attachments (aka "render targets"), 
		* depth-stencil attachments, images/buffers used as storage image/buffer (aka "Unordered Access View (UAV)").
		*/
		GPUOnly,

		/*
		* Buffers for data written by or transferred from the GPU that you want to read back on the CPU, 
		* e.g. results of some computations.
		*/
		Readback
	};

	class VulkanAllocator {
	public:
		VulkanAllocator(const VulkanAllocator&) = delete;
		VulkanAllocator& operator=(const VulkanAllocator&) = delete;
		VulkanAllocator(VulkanAllocator&&) = delete;
		VulkanAllocator& operator=(VulkanAllocator&&) = delete;

		VmaAllocationInfo CreateVulkanBufferVma(MemoryUsage lucyBufferUsage, VkDeviceSize size, VkBufferUsageFlags usage,
			bool persistentlyMapped, VkBuffer& bufferHandle, VmaAllocation& vmaAllocation, VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE);

		VmaAllocationInfo CreateVulkanImageVma(uint32_t width, uint32_t height, uint32_t mipLevel, VkFormat format, VkImageLayout currentLayout, VkImageUsageFlags usage,
								  VkImageType imageType, VkImage& imageHandle, VmaAllocation& allocationHandle, VkImageCreateFlags flags = 0, uint32_t arrayLayer = 1, VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE);

		void MapMemory(VmaAllocation allocation, void*& mappedData);
		void UnmapMemory(VmaAllocation allocation);

		void Flush(VmaAllocation allocation, VkDeviceSize offset, VkDeviceSize size);

		void DestroyBuffer(VkBuffer buffer, VmaAllocation allocation);
		void DestroyImage(VkImage buffer, VmaAllocation allocation);
	private:
		VulkanAllocator() = default;
		~VulkanAllocator() = default;

		[[deprecated("Old way, prefer vma!")]]
		void CreateVulkanBuffer(uint32_t size, VkBufferUsageFlags usage, VkSharingMode sharingMode, uint32_t memProperties, VkBuffer& bufferHandle, VkDeviceMemory& memory);
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags propertyFlags);

		void Init(VkInstance instance, VulkanRenderDevice* device, uint32_t apiVersion);
		void Destroy();

		VmaAllocator m_Allocator;
		
		VulkanRenderDevice* m_RenderDevice = nullptr;

		friend class VulkanRenderDevice;
	};
}
