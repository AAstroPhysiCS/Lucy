#include "lypch.h"
#include "VulkanAllocator.h"

#include "Renderer/Context/VulkanContextDevice.h"

#define VMA_IMPLEMENTATION

namespace Lucy {
	
	VulkanAllocator& VulkanAllocator::Get() {
		static VulkanAllocator s_Instance;
		return s_Instance;
	}

	void VulkanAllocator::Init(VkInstance instance) {
		VulkanContextDevice& device = VulkanContextDevice::Get();

		VmaAllocatorCreateInfo createInfo{};
		createInfo.vulkanApiVersion = VK_API_VERSION_1_3;
		createInfo.physicalDevice = device.GetPhysicalDevice();
		createInfo.device = device.GetLogicalDevice();
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
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = sharingMode;

		VkDevice device = VulkanContextDevice::Get().GetLogicalDevice();

		LUCY_VK_ASSERT(vkCreateBuffer(device, &bufferInfo, nullptr, &bufferHandle));

		VkMemoryRequirements memoryRequirements{};
		vkGetBufferMemoryRequirements(device, bufferHandle, &memoryRequirements);

		VkMemoryAllocateInfo memoryAllocInfo{};
		memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocInfo.allocationSize = memoryRequirements.size;
		memoryAllocInfo.memoryTypeIndex = FindMemoryType(memoryRequirements.memoryTypeBits, memProperties);

		LUCY_VK_ASSERT(vkAllocateMemory(device, &memoryAllocInfo, nullptr, &memory));

		vkBindBufferMemory(device, bufferHandle, memory, 0);
	}

	uint32_t VulkanAllocator::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags propertyFlags) {
		VkPhysicalDevice physicalDevice = VulkanContextDevice::Get().GetPhysicalDevice();

		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			bool suitableMemoryType = typeFilter & (1 << i);
			bool suitableMemoryProperties = (memProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags; //Host visible and host coherent
			if (suitableMemoryType && suitableMemoryProperties) {
				return i;
			}
		}

		LUCY_ASSERT(false);
		LUCY_INFO("Cant find a suitable memory type or property");
		return 0;
	}

	void VulkanAllocator::CreateVulkanBufferVma(VulkanBufferUsage lucyBufferUsage, VkDeviceSize size, VkBufferUsageFlags usage,
												VkBuffer& bufferHandle, VmaAllocation& vmaAllocation) {
		VkBufferCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		createInfo.size = size;
		createInfo.usage = usage;

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
		VkImageCreateInfo imageCreateInfo{};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = imageType;
		imageCreateInfo.extent.width = width;
		imageCreateInfo.extent.height = height;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.mipLevels = mipLevel;
		imageCreateInfo.arrayLayers = arrayLayers;
		imageCreateInfo.format = format;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.initialLayout = currentLayout;
		imageCreateInfo.usage = usage;
		imageCreateInfo.flags = flags;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT; //no multisampling

		VmaAllocationCreateInfo allocationCreateInfo{};
		allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocationCreateInfo.flags = 0;

		LUCY_VK_ASSERT(vmaCreateImage(m_Allocator, &imageCreateInfo, &allocationCreateInfo, &imageHandle, &allocationHandle, nullptr));
	}
}