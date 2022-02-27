#pragma once

#include "vulkan/vulkan.h"
#include "../UniformBuffer.h"
#include "../../VulkanDescriptors.h"

namespace Lucy {

	class VulkanUniformBuffer : public UniformBuffer {
	public:
		VulkanUniformBuffer(uint32_t size, uint32_t binding, VulkanDescriptorSet& descriptorSet);
		virtual ~VulkanUniformBuffer() = default;

		void Bind();
		void Unbind();
		void Destroy();
		void SetData(void* data, uint32_t size, uint32_t offset);
		void WriteToSets(uint32_t index);

		inline VulkanDescriptorSet& GetDescriptorSet() noexcept { return m_DescriptorSet; }
	private:
		uint32_t m_Binding = 0;
		VulkanDescriptorSet m_DescriptorSet;

		std::vector<VkBuffer> m_Buffers;
		std::vector<VkDeviceMemory> m_BufferMemories;
	};

}

