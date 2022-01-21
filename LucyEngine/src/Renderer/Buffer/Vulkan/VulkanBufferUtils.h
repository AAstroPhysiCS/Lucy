#pragma once

#include "vulkan/vulkan.h"

namespace Lucy {

	class VulkanBufferUtils {
		static void CreateVulkanBuffer(uint32_t size, VkBufferUsageFlags usage, VkSharingMode sharingMode, uint32_t memProperties, VkBuffer& bufferHandle, VkDeviceMemory& memory);
		static uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags propertyFlags);

		friend class VulkanIndexBuffer;
		friend class VulkanVertexBuffer;
		friend class VulkanUniformBuffer;

		VulkanBufferUtils() = delete;
		~VulkanBufferUtils() = delete;
	};
}

