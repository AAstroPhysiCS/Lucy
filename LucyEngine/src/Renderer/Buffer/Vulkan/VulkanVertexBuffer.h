#pragma once

#include "../VertexBuffer.h"

#include "vulkan/vulkan.h"
#include "vma/vk_mem_alloc.h"

namespace Lucy {

	class VulkanVertexBuffer : public VertexBuffer {
	public:
		VulkanVertexBuffer(uint32_t size);
		VulkanVertexBuffer();
		virtual ~VulkanVertexBuffer() = default;

		void Bind(const VertexBindInfo& info) override;
		void Unbind() override;
		void AddData(const std::vector<float>& dataToAdd) override;
		void Load() override;
		void Destroy() override;
	private:
		void Create(uint32_t size = 0);

		VkBuffer m_StagingBufferHandle = VK_NULL_HANDLE;
		VkBuffer m_BufferHandle = VK_NULL_HANDLE;
		VmaAllocation m_StagingBufferVma = VK_NULL_HANDLE;
		VmaAllocation m_BufferVma = VK_NULL_HANDLE;
	};
}