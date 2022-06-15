#pragma once

#include "vulkan/vulkan.h"
#include "vma/vk_mem_alloc.h"

#include "../UniformBuffer.h"
#include "../../VulkanDescriptors.h"
#include "../../Image/VulkanImage.h"

namespace Lucy {

	class VulkanUniformBuffer : public UniformBuffer {
	public:
		VulkanUniformBuffer(uint32_t size, uint32_t binding, VulkanDescriptorSet& descriptorSet);
		virtual ~VulkanUniformBuffer() = default;

		void Bind() override;
		void Unbind() override;
		void Destroy() override;
		void SetData(void* data, uint32_t size, uint32_t offset) override;
		void Update(RefLucy<VulkanImage2D> image = nullptr);

		inline VulkanDescriptorSet& GetDescriptorSet() noexcept { return m_DescriptorSet; }
	private:
		uint32_t m_Binding = 0;
		VulkanDescriptorSet m_DescriptorSet;

		std::vector<VkBuffer> m_Buffers;
		std::vector<VmaAllocation> m_BufferVma;

		bool m_Allocated = false;
	};
}