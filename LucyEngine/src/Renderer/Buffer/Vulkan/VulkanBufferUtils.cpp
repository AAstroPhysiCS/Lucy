#include "lypch.h"
#include "VulkanBufferUtils.h"

#include "Renderer/Context/VulkanDevice.h"

namespace Lucy {

	void VulkanBufferUtils::CreateVulkanBuffer(uint32_t size, VkBufferUsageFlags usage, VkSharingMode sharingMode,
										  uint32_t memProperties, VkBuffer& bufferHandle, VkDeviceMemory& memory) {
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(float) * size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = sharingMode;

		VkDevice device = VulkanDevice::Get().GetLogicalDevice();

		LUCY_VULKAN_ASSERT(vkCreateBuffer(device, &bufferInfo, nullptr, &bufferHandle));
		
		VkMemoryRequirements memoryRequirements{};
		vkGetBufferMemoryRequirements(device, bufferHandle, &memoryRequirements);
		VkMemoryAllocateInfo memoryAllocInfo{};
		memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocInfo.allocationSize = memoryRequirements.size;
		memoryAllocInfo.memoryTypeIndex = FindMemoryType(memoryRequirements.memoryTypeBits, memProperties);

		LUCY_VULKAN_ASSERT(vkAllocateMemory(device, &memoryAllocInfo, nullptr, &memory));

		vkBindBufferMemory(device, bufferHandle, memory, 0);
	}

	uint32_t VulkanBufferUtils::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags propertyFlags) {
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
	}

}