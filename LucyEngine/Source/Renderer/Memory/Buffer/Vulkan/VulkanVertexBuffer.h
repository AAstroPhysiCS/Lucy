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

		VulkanVertexBuffer(const VulkanVertexBuffer&) = delete;
		VulkanVertexBuffer& operator=(const VulkanVertexBuffer&) = delete;
		VulkanVertexBuffer(VulkanVertexBuffer&&) = delete;
		VulkanVertexBuffer& operator=(VulkanVertexBuffer&&) = delete;

		void RTBind(const VulkanVertexBindInfo& info);
		void RTLoadToDevice(const Ref<RenderDevice>& device) final override;
	private:
		void RTDestroyResource(RenderDevice* device) final override;
		void RTCreate(const Ref<VulkanRenderDevice>& device, size_t size);

		VkBuffer m_BufferHandle = VK_NULL_HANDLE;
		VmaAllocation m_BufferVma = VK_NULL_HANDLE;
	};
}