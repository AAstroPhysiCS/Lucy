#pragma once

#include "vulkan/vulkan.h"
#include "../UniformBuffer.h"
#include "../../VulkanDescriptors.h"

namespace Lucy {

	class VulkanUniformBuffer : public UniformBuffer {
	public:
		VulkanUniformBuffer(uint32_t size, uint32_t binding);
		virtual ~VulkanUniformBuffer() = default;

		void Bind();
		void Unbind();
		void Destroy();
		void SetData(void* data, uint32_t size, uint32_t offset);
	private:
		RefLucy<VulkanDescriptorSet> m_DescriptorSet;

		VkBuffer m_Buffer = VK_NULL_HANDLE;
		VkDeviceMemory m_BufferMemory = VK_NULL_HANDLE;
	};

}

