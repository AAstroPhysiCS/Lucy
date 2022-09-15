#pragma once

#include "vma/vk_mem_alloc.h"

namespace Lucy {

	enum class VulkanBufferUsage {
		/*
		* Auto, with no additional flag
		*/
		Auto,

		/*
		* A buffer that u can map with vmaMapMemory.
		* Resides on the CPU Ram
		*/
		CPUOnly,
		
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
		static VulkanAllocator& Get();
		
		void Init(VkInstance instance);
		void Destroy();
		
		void CreateVulkanBufferVma(VulkanBufferUsage lucyBufferUsage, uint32_t size,
								   VkBufferUsageFlags usage, VkBuffer& bufferHandle, VmaAllocation& vmaAllocation);

		void CreateVulkanImageVma(uint32_t width, uint32_t height, uint32_t mipLevel, VkFormat format, VkImageLayout currentLayout, VkImageUsageFlags usage, 
								  VkImageType imageType, VkImage& imageHandle, VmaAllocation& allocationHandle, VkImageCreateFlags flags = 0, uint32_t arrayLayer = 1);

		void MapMemory(VmaAllocation allocation, void*& mappedData);
		void UnmapMemory(VmaAllocation allocation);

		void DestroyBuffer(VkBuffer buffer, VmaAllocation allocation);
		void DestroyImage(VkImage buffer, VmaAllocation allocation);
	private:
		VulkanAllocator() = default;
		~VulkanAllocator() = default;

		[[deprecated("Old way, prefer vma!")]]
		void CreateVulkanBuffer(uint32_t size, VkBufferUsageFlags usage, VkSharingMode sharingMode, uint32_t memProperties, VkBuffer& bufferHandle, VkDeviceMemory& memory);
		[[deprecated("Old way, prefer vma!")]]
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags propertyFlags);

		VmaAllocator m_Allocator;
	};
}
