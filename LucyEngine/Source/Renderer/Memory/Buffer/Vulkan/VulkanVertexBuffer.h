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
		VulkanVertexBuffer(size_t size, const Ref<VulkanRenderDevice>& device);
		virtual ~VulkanVertexBuffer() = default;

		void RTBind(const VulkanVertexBindInfo& info);
		void RTLoadToDevice() final override;
	private:
		void RTDestroyResource() final override;
		void RTCreate(size_t size);

		VkBuffer m_StagingBufferHandle = VK_NULL_HANDLE;
		VkBuffer m_BufferHandle = VK_NULL_HANDLE;

		VmaAllocation m_StagingBufferVma = VK_NULL_HANDLE;
		VmaAllocation m_BufferVma = VK_NULL_HANDLE;

		Ref<VulkanRenderDevice> m_VulkanDevice = nullptr;
	};
}