#pragma once

#include "vulkan/vulkan.h"
#include "../UniformBuffer.h"

namespace Lucy {

	class VulkanUniformBuffer : UniformBuffer {
	public:
		VulkanUniformBuffer(uint32_t size, uint32_t binding);
		~VulkanUniformBuffer() = default;

		void Bind();
		void Unbind();
		void Destroy();
		void SetData(void* data, uint32_t size, uint32_t offset);
	private:
		VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
		VkBuffer m_Buffer = VK_NULL_HANDLE;
		VkDeviceMemory m_BufferMemory = VK_NULL_HANDLE;
	};

	class VulkanUniformPool {
	public:
		VulkanUniformPool() = default;
		~VulkanUniformPool() = default;
	};
}

