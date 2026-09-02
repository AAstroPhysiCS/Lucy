#include "lypch.h"
#include "VulkanAllocator.h"

#include "Renderer/Device/VulkanRenderDevice.h"

#define VMA_IMPLEMENTATION
//#define VMA_DEBUG_LOG_FORMAT(format, ...) do { \
//	   printf((format), __VA_ARGS__); \
//	   printf("\n"); \
//   } while(false)
//#define VMA_LEAK_LOG_FORMAT(format, ...) do { \
//        printf((format), __VA_ARGS__); \
//        printf("\n"); \
//    } while(false)
#include "vma/vk_mem_alloc.h"

namespace Lucy {
	
	void VulkanAllocator::Init(VkInstance instance, VulkanRenderDevice* device, uint32_t apiVersion) {
		m_RenderDevice = device;

		VmaAllocatorCreateInfo createInfo{};
		createInfo.vulkanApiVersion = apiVersion;
		createInfo.physicalDevice = m_RenderDevice->GetPhysicalDevice();
		createInfo.device = m_RenderDevice->GetLogicalDevice();
		createInfo.instance = instance;
		createInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

		LUCY_VK_ASSERT(vmaCreateAllocator(&createInfo, &m_Allocator));
	}

	void VulkanAllocator::MapMemory(VmaAllocation allocation, void*& mappedData) {
		vmaMapMemory(m_Allocator, allocation, &mappedData);
	}

	void VulkanAllocator::UnmapMemory(VmaAllocation allocation) {
		vmaUnmapMemory(m_Allocator, allocation);
	}

	void VulkanAllocator::Flush(VmaAllocation allocation, VkDeviceSize offset, VkDeviceSize size) {
		vmaFlushAllocation(m_Allocator, allocation, offset, size);
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
		VkDevice logicalDevice = m_RenderDevice->GetLogicalDevice();

		VkBufferCreateInfo bufferInfo = VulkanAPI::BufferCreateInfo(size, usage, sharingMode);
		LUCY_VK_ASSERT(vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &bufferHandle));

		VkMemoryRequirements memoryRequirements{};
		vkGetBufferMemoryRequirements(logicalDevice, bufferHandle, &memoryRequirements);

		VkMemoryAllocateInfo memoryAllocInfo{};
		memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocInfo.allocationSize = memoryRequirements.size;
		memoryAllocInfo.memoryTypeIndex = FindMemoryType(memoryRequirements.memoryTypeBits, memProperties);

		LUCY_VK_ASSERT(vkAllocateMemory(logicalDevice, &memoryAllocInfo, nullptr, &memory));

		vkBindBufferMemory(logicalDevice, bufferHandle, memory, 0);
	}

	uint32_t VulkanAllocator::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags propertyFlags) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(m_RenderDevice->GetPhysicalDevice(), &memProperties);

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

	VmaAllocationInfo VulkanAllocator::CreateVulkanBufferVma(MemoryUsage lucyBufferUsage, VkDeviceSize size, VkBufferUsageFlags usage,
												bool persistentlyMapped, VkBuffer& bufferHandle, VmaAllocation& vmaAllocation, VkSharingMode sharingMode) {
		VkBufferCreateInfo createInfo{};
		
		uint32_t queueFamilyIndices[] = {
			m_RenderDevice->GetQueueFamilies().GraphicsFamily,
			m_RenderDevice->GetQueueFamilies().ComputeFamily,
			m_RenderDevice->GetQueueFamilies().TransferFamily
		};

		if (sharingMode == VK_SHARING_MODE_CONCURRENT)
			createInfo = VulkanAPI::BufferCreateInfo(size, usage, VK_SHARING_MODE_CONCURRENT, static_cast<uint32_t>(TargetQueueFamily::Count), queueFamilyIndices);
		else
			createInfo = VulkanAPI::BufferCreateInfo(size, usage, VK_SHARING_MODE_EXCLUSIVE);

		VmaAllocationCreateInfo vmaCreateInfo{};
		vmaCreateInfo.priority = 1.0f;

		switch (lucyBufferUsage) {
			case MemoryUsage::Auto:
				vmaCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
				vmaCreateInfo.flags = 0;
				break;
			case MemoryUsage::CPUOnly:
				vmaCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
				vmaCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
				break;
			case MemoryUsage::GPUOnly:
				vmaCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
				//vmaCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
				break;
			case MemoryUsage::CPUToGPU:
				vmaCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
				vmaCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
				break;
			case MemoryUsage::Readback:
				vmaCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
				vmaCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
				break;
		}

		if (persistentlyMapped)
			vmaCreateInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;

		VmaAllocationInfo resultInfo{};
		LUCY_VK_ASSERT(vmaCreateBuffer(m_Allocator, &createInfo, &vmaCreateInfo, &bufferHandle, &vmaAllocation, &resultInfo));

		return resultInfo;
	}

	VmaAllocationInfo VulkanAllocator::CreateVulkanImageVma(uint32_t width, uint32_t height, uint32_t mipLevel, VkFormat format, VkImageLayout currentLayout, VkImageUsageFlags usage,
											   VkImageType imageType, VkImage& imageHandle, VmaAllocation& allocationHandle, VkImageCreateFlags flags, uint32_t arrayLayers, VkSharingMode sharingMode) {
		VkImageCreateInfo imageCreateInfo{};

		uint32_t queueFamilyIndices[] = {
			m_RenderDevice->GetQueueFamilies().GraphicsFamily,
			m_RenderDevice->GetQueueFamilies().ComputeFamily,
			m_RenderDevice->GetQueueFamilies().TransferFamily
		};

		if (sharingMode == VK_SHARING_MODE_CONCURRENT)
			imageCreateInfo = VulkanAPI::ImageCreateInfo(imageType, { width, height, 1 }, mipLevel, arrayLayers, format, VK_IMAGE_TILING_OPTIMAL, currentLayout, usage, VK_SHARING_MODE_CONCURRENT, VK_SAMPLE_COUNT_1_BIT, static_cast<uint32_t>(TargetQueueFamily::Count), queueFamilyIndices, flags);
		else
			imageCreateInfo = VulkanAPI::ImageCreateInfo(imageType, { width, height, 1 }, mipLevel, arrayLayers, format, VK_IMAGE_TILING_OPTIMAL, currentLayout, usage, VK_SHARING_MODE_EXCLUSIVE, VK_SAMPLE_COUNT_1_BIT, 0, nullptr, flags);
		
		VmaAllocationCreateInfo allocationCreateInfo{};
		allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocationCreateInfo.flags = 0;

		VmaAllocationInfo resultInfo{};
		LUCY_VK_ASSERT(vmaCreateImage(m_Allocator, &imageCreateInfo, &allocationCreateInfo, &imageHandle, &allocationHandle, &resultInfo));
		return resultInfo;
	}
}