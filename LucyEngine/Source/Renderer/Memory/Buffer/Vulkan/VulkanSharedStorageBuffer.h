#pragma once

#include "vulkan/vulkan.h"
#include "vma/vk_mem_alloc.h"

#include "../SharedStorageBuffer.h"

namespace Lucy {

	class VulkanSharedStorageBuffer : public SharedStorageBuffer {
	public:
		VulkanSharedStorageBuffer(const SharedStorageBufferCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device);
		virtual ~VulkanSharedStorageBuffer() = default;

		VulkanSharedStorageBuffer(const VulkanSharedStorageBuffer&) = delete;
		VulkanSharedStorageBuffer& operator=(const VulkanSharedStorageBuffer&) = delete;
		VulkanSharedStorageBuffer(VulkanSharedStorageBuffer&&) = delete;
		VulkanSharedStorageBuffer& operator=(VulkanSharedStorageBuffer&&) = delete;

		void RTLoadToDevice(RenderDevice* device) final override;

		inline VkBuffer GetVulkanBufferHandle(const uint32_t index) { return m_Buffers[index]; }
	private:
		void RTDestroyResource(RenderDevice* device) final override;

		std::vector<VkBuffer> m_Buffers;
		std::vector<VmaAllocation> m_BufferVma;
	};
}

