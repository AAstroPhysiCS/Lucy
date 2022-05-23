#include "lypch.h"
#include "VulkanAllocator.h"

#include "Renderer/Context/VulkanDevice.h"

#define VMA_IMPLEMENTATION

namespace Lucy {
	
	VulkanAllocator& VulkanAllocator::Get() {
		static VulkanAllocator s_Instance;
		return s_Instance;
	}

	void VulkanAllocator::Init(VkInstance instance) {
		VulkanDevice& device = VulkanDevice::Get();

		VmaAllocatorCreateInfo createInfo{};
		createInfo.vulkanApiVersion = VK_API_VERSION_1_2;
		createInfo.physicalDevice = device.GetPhysicalDevice();
		createInfo.device = device.GetLogicalDevice();
		createInfo.instance = instance;

		LUCY_VK_ASSERT(vmaCreateAllocator(&createInfo, &m_Allocator));
	}

	void VulkanAllocator::CreateVulkanBuffer(uint32_t size, VkBufferUsageFlags usage, VkSharingMode sharingMode,
											   uint32_t memProperties, VkBuffer& bufferHandle, VkDeviceMemory& memory) {
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = sharingMode;

		VkDevice device = VulkanDevice::Get().GetLogicalDevice();

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
		VkPhysicalDevice physicalDevice = VulkanDevice::Get().GetPhysicalDevice();

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

	void VulkanAllocator::CreateVulkanBufferVma(uint32_t size, VkBufferUsageFlags usage, VmaMemoryUsage vmaMemoryUsage, VkBuffer& bufferHandle, VmaAllocation& vmaAllocation) {
		VkBufferCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		createInfo.size = size;
		createInfo.usage = usage;

		VmaAllocationCreateInfo vmaCreateInfo{};
		vmaCreateInfo.usage = vmaMemoryUsage;
		vmaCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

		LUCY_VK_ASSERT(vmaCreateBuffer(m_Allocator, &createInfo, &vmaCreateInfo, &bufferHandle, &vmaAllocation, nullptr));
	}

	void VulkanAllocator::CreateVulkanImageVma(uint32_t width, uint32_t height, VkFormat format, VkImageLayout currentLayout, VkImageUsageFlags usage, VkImageType imageType,
											   VmaMemoryUsage memUsage, VkImage& imageHandle, VmaAllocation& allocationHandle) {
		VkImageCreateInfo imageCreateInfo{};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = imageType;
		imageCreateInfo.extent.width = width;
		imageCreateInfo.extent.height = height;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.mipLevels = 1; //TODO: no mipmapping yet
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.format = format;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.initialLayout = currentLayout;
		imageCreateInfo.usage = usage;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT; //no multisampling

		VmaAllocationCreateInfo allocationCreateInfo{};
		allocationCreateInfo.usage = memUsage;

		LUCY_VK_ASSERT(vmaCreateImage(m_Allocator, &imageCreateInfo, &allocationCreateInfo, &imageHandle, &allocationHandle, nullptr));
	}
}