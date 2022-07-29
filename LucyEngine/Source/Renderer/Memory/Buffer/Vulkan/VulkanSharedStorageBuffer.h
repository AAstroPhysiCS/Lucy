#pragma once

#include "vulkan/vulkan.h"
#include "vma/vk_mem_alloc.h"

#include "../SharedStorageBuffer.h"

namespace Lucy {

	class VulkanSharedStorageBuffer : public SharedStorageBuffer {
	public:
		VulkanSharedStorageBuffer(const SharedStorageBufferCreateInfo& createInfo);
		virtual ~VulkanSharedStorageBuffer() = default;

		void LoadToGPU() final override;
		void DestroyHandle() final override;

		inline VkBuffer GetVulkanBufferHandle(const uint32_t index) { return m_Buffers[index]; }
	private:
		std::vector<VkBuffer> m_Buffers;
		std::vector<VmaAllocation> m_BufferVma;
	};
}

