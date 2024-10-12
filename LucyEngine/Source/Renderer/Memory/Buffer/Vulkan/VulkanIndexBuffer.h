#pragma once

#include "../IndexBuffer.h"
#include "vma/vk_mem_alloc.h"

namespace Lucy {

	struct VulkanIndexBindInfo {
		VkCommandBuffer CommandBuffer;
	};

	class VulkanIndexBuffer : public IndexBuffer {
	public:
		VulkanIndexBuffer(size_t size, const Ref<VulkanRenderDevice>& device);
		virtual ~VulkanIndexBuffer() = default;
		
		void RTBind(const VulkanIndexBindInfo& info);
		void RTLoadToDevice() final override;
	private:
		void RTCreate(size_t size = 0);
		void RTDestroyResource() final override;

		VkBuffer m_BufferHandle = VK_NULL_HANDLE;
		VmaAllocation m_BufferVma = VK_NULL_HANDLE;

		VkBuffer m_StagingBufferHandle = VK_NULL_HANDLE;
		VmaAllocation m_StagingBufferVma = VK_NULL_HANDLE;

		Ref<VulkanRenderDevice> m_VulkanDevice = nullptr;
	};
}

