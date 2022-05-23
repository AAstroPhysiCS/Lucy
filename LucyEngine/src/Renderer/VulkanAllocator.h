#pragma once

#include "vma/vk_mem_alloc.h"

namespace Lucy {

	class VulkanAllocator {
	public:
		static VulkanAllocator& Get();
		
		void Init(VkInstance instance);
		void CreateVulkanBufferVma(uint32_t size, VkBufferUsageFlags usage, VmaMemoryUsage vmaMemoryUsage, VkBuffer& bufferHandle, VmaAllocation& vmaAllocation);
		void CreateVulkanImageVma(uint32_t width, uint32_t height, VkFormat format, VkImageLayout currentLayout, VkImageUsageFlags usage, VkImageType imageType, VmaMemoryUsage memUsage, VkImage& imageHandle, VmaAllocation& allocationHandle);

		//old way, prefer vma
		void CreateVulkanBuffer(uint32_t size, VkBufferUsageFlags usage, VkSharingMode sharingMode, uint32_t memProperties, VkBuffer& bufferHandle, VkDeviceMemory& memory);
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags propertyFlags);

		inline VmaAllocator GetVmaInstance() { return m_Allocator; }
	private:
		VulkanAllocator() = default;
		~VulkanAllocator() = default;

		VmaAllocator m_Allocator;
	};
}
