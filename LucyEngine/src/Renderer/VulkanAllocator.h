#pragma once

#include "vma/vk_mem_alloc.h"

namespace Lucy {

	class VulkanAllocator {
	public:
		static VulkanAllocator& Get();
		
		void Init();
		void CreateVulkanBufferVMA(uint32_t size, VkBufferUsageFlags usage, VmaMemoryUsage vmaMemoryUsage, VkBuffer& bufferHandle, VmaAllocation& vmaAllocation);

		//old way, prefer vma
		void CreateVulkanBuffer(uint32_t size, VkBufferUsageFlags usage, VkSharingMode sharingMode, uint32_t memProperties, VkBuffer& bufferHandle, VkDeviceMemory& memory);
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags propertyFlags);

		inline VmaAllocator GetVMAInstance() { return m_Allocator; }
	private:
		VulkanAllocator() = default;
		~VulkanAllocator() = default;

		VmaAllocator m_Allocator;
	};
}

