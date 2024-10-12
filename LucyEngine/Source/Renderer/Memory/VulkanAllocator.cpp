#include "lypch.h"
#include "VulkanAllocator.h"

#define VMA_IMPLEMENTATION

namespace Lucy {
	
	void VulkanAllocator::Init(VkInstance instance, VkDevice logicalDevice, VkPhysicalDevice physicalDevice, uint32_t apiVersion) {
		m_LogicalDevice = logicalDevice;
		m_PhysicalDevice = physicalDevice;

		VmaAllocatorCreateInfo createInfo{};
		createInfo.vulkanApiVersion = apiVersion;
		createInfo.physicalDevice = m_PhysicalDevice;
		createInfo.device = m_LogicalDevice;
		createInfo.instance = instance;

		LUCY_VK_ASSERT(vmaCreateAllocator(&createInfo, &m_Allocator));
	}

	void VulkanAllocator::MapMemory(VmaAllocation allocation, void*& mappedData) {
		vmaMapMemory(m_Allocator, allocation, &mappedData);
	}

	void VulkanAllocator::UnmapMemory(VmaAllocation allocation) {
		vmaUnmapMemory(m_Allocator, allocation);
	}

	void VulkanAllocator::DestroyBuffer(VkBuffer buffer, VmaAllocation allocation) {
		vmaDestroyBuffer(m_Allocator, buffer, allocation);
	}

	void VulkanAllocator::DestroyImage(VkImage image, VmaAllocation allocation) {
		vmaDestroyImage(m_Allocator, image, allocation);
	}

	void VulkanAllocator::Destroy() {
		vmaDestroyAllocator(m_Allocator);
	}

	void VulkanAllocator::CreateVulkanBuffer(uint32_t size, VkBufferUsageFlags usage, VkSharingMode sharingMode,
											   uint32_t memProperties, VkBuffer& bufferHandle, VkDeviceMemory& memory) {
		VkBufferCreateInfo bufferInfo = VulkanAPI::BufferCreateInfo(size, usage, sharingMode);
		LUCY_VK_ASSERT(vkCreateBuffer(m_LogicalDevice, &bufferInfo, nullptr, &bufferHandle));

		VkMemoryRequirements memoryRequirements{};
		vkGetBufferMemoryRequirements(m_LogicalDevice, bufferHandle, &memoryRequirements);

		VkMemoryAllocateInfo memoryAllocInfo{};
		memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocInfo.allocationSize = memoryRequirements.size;
		memoryAllocInfo.memoryTypeIndex = FindMemoryType(memoryRequirements.memoryTypeBits, memProperties);

		LUCY_VK_ASSERT(vkAllocateMemory(m_LogicalDevice, &memoryAllocInfo, nullptr, &memory));

		vkBindBufferMemory(m_LogicalDevice, bufferHandle, memory, 0);
	}

	uint32_t VulkanAllocator::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags propertyFlags) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			bool suitableMemoryType = typeFilter & (1 << i);
			bool suitableMemoryProperties = (memProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags; //Host visible and host coherent
			if (suitableMemoryType && suitableMemoryProperties) {
				return i;
			}
		}

		LUCY_ASSERT(false, "Cant find a suitable memory type or property");
		return 0;
	}

	void VulkanAllocator::CreateVulkanBufferVma(VulkanBufferUsage lucyBufferUsage, VkDeviceSize size, VkBufferUsageFlags usage,
												VkBuffer& bufferHandle, VmaAllocation& vmaAllocation) {
		VkBufferCreateInfo createInfo = VulkanAPI::BufferCreateInfo(size, usage, VK_SHARING_MODE_EXCLUSIVE);

		VmaAllocationCreateInfo vmaCreateInfo{};
		vmaCreateInfo.priority = 1.0f;

		switch (lucyBufferUsage) {
			case VulkanBufferUsage::Auto:
				vmaCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
				vmaCreateInfo.flags = 0;
				break;
			case VulkanBufferUsage::CPUOnly:
				vmaCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
				vmaCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
				break;
			case VulkanBufferUsage::GPUOnly:
				vmaCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
				//vmaCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
				break;
			case VulkanBufferUsage::Readback:
				vmaCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
				vmaCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
				break;
		}

		LUCY_VK_ASSERT(vmaCreateBuffer(m_Allocator, &createInfo, &vmaCreateInfo, &bufferHandle, &vmaAllocation, nullptr));
	}

	void VulkanAllocator::CreateVulkanImageVma(uint32_t width, uint32_t height, uint32_t mipLevel, VkFormat format, VkImageLayout currentLayout, VkImageUsageFlags usage,
											   VkImageType imageType, VkImage& imageHandle, VmaAllocation& allocationHandle, VkImageCreateFlags flags, uint32_t arrayLayers) {
		VkImageCreateInfo imageCreateInfo = VulkanAPI::ImageCreateInfo(imageType, { width, height, 1 }, mipLevel, arrayLayers, 
																	   format, VK_IMAGE_TILING_OPTIMAL, currentLayout, usage, 
																	   VK_SHARING_MODE_EXCLUSIVE, VK_SAMPLE_COUNT_1_BIT, flags);
		VmaAllocationCreateInfo allocationCreateInfo{};
		allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocationCreateInfo.flags = 0;

		LUCY_VK_ASSERT(vmaCreateImage(m_Allocator, &imageCreateInfo, &allocationCreateInfo, &imageHandle, &allocationHandle, nullptr));
	}
}