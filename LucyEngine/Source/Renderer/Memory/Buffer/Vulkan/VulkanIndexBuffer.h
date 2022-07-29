#pragma once

#include "../IndexBuffer.h"
#include "vma/vk_mem_alloc.h"

namespace Lucy {

	class VulkanIndexBuffer : public IndexBuffer {
	public:
		VulkanIndexBuffer(uint32_t size);
		virtual ~VulkanIndexBuffer() = default;
		
		void Bind(const IndexBindInfo& info) final override;
		void Unbind() final override;
		void LoadToGPU() final override;
		void DestroyHandle() final override;
	private:
		void Create(uint32_t size = 0);

		VkBuffer m_BufferHandle = VK_NULL_HANDLE;
		VmaAllocation m_BufferVma = VK_NULL_HANDLE;

		VkBuffer m_StagingBufferHandle = VK_NULL_HANDLE;
		VmaAllocation m_StagingBufferVma = VK_NULL_HANDLE;
	};
}

