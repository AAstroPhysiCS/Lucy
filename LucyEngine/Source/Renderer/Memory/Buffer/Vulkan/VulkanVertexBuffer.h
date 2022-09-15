#pragma once

#include "../VertexBuffer.h"

#include "vulkan/vulkan.h"
#include "vma/vk_mem_alloc.h"

namespace Lucy {

	struct VulkanVertexBindInfo {
		VkCommandBuffer CommandBuffer;
	};

	class VulkanVertexBuffer : public VertexBuffer {
	public:
		VulkanVertexBuffer(uint32_t size);
		virtual ~VulkanVertexBuffer() = default;

		void Bind(const VulkanVertexBindInfo& info);
		void LoadToGPU() final override;
		void DestroyHandle() final override;
	private:
		void Create(uint32_t size);

		VkBuffer m_StagingBufferHandle = VK_NULL_HANDLE;
		VkBuffer m_BufferHandle = VK_NULL_HANDLE;

		VmaAllocation m_StagingBufferVma = VK_NULL_HANDLE;
		VmaAllocation m_BufferVma = VK_NULL_HANDLE;
	};
}