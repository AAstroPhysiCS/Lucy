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

		VulkanIndexBuffer(const VulkanIndexBuffer&) = delete;
		VulkanIndexBuffer& operator=(const VulkanIndexBuffer&) = delete;
		VulkanIndexBuffer(VulkanIndexBuffer&&) = delete;
		VulkanIndexBuffer& operator=(VulkanIndexBuffer&&) = delete;
		
		void RTBind(const VulkanIndexBindInfo& info);
		void RTLoadToDevice(RenderDevice* device) final override;
	private:
		void RTCreate(const Ref<VulkanRenderDevice>& device, size_t size = 0);
		void RTDestroyResource(RenderDevice* device) final override;

		VkBuffer m_BufferHandle = VK_NULL_HANDLE;
		VmaAllocation m_BufferVma = VK_NULL_HANDLE;
	};
}

