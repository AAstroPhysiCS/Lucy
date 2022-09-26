#pragma once

#include "vulkan/vulkan.h"
#include "CommandPool.h"

namespace Lucy {

	class VulkanCommandPool : public CommandPool {
	public:
		VulkanCommandPool(const CommandPoolCreateInfo& createInfo);
		virtual ~VulkanCommandPool() = default;

		inline VkCommandBuffer GetCommandBuffer(uint32_t index) { return m_CommandBuffers[index]; }
		inline size_t GetCommandBufferSize() { return m_CommandBuffers.size(); }

		void Destroy() final override;
		void Recreate() final override;
	private:
		void Allocate() final override;
		void FreeCommandBuffers(uint32_t commandBufferCount, uint32_t commandBufferStartIndex);

		VkCommandBuffer BeginSingleTimeCommand();
		void EndSingleTimeCommand();

		VkCommandPool m_CommandPool = VK_NULL_HANDLE;
		std::vector<VkCommandBuffer> m_CommandBuffers;

		friend class VulkanCommandQueue; //for singletime commands
	};
}

