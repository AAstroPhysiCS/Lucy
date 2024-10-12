#pragma once

#include "vulkan/vulkan.h"
#include "vma/vk_mem_alloc.h"

#include "../SharedStorageBuffer.h"

namespace Lucy {

	class VulkanSharedStorageBuffer : public SharedStorageBuffer {
	public:
		VulkanSharedStorageBuffer(const SharedStorageBufferCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device);
		virtual ~VulkanSharedStorageBuffer() = default;

		void RTLoadToDevice() final override;

		inline VkBuffer GetVulkanBufferHandle(const uint32_t index) { return m_Buffers[index]; }
	private:
		void RTDestroyResource() final override;

		std::vector<VkBuffer> m_Buffers;
		std::vector<VmaAllocation> m_BufferVma;

		Ref<VulkanRenderDevice> m_VulkanDevice = nullptr;
	};
}

