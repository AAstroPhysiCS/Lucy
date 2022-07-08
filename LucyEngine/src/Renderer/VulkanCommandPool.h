#pragma once

#include "vulkan/vulkan.h"

namespace Lucy {

	class VulkanPipeline;

	struct CommandPoolCreateInfo {
		VkCommandPoolCreateFlags PoolFlags = 0;
		VkCommandBufferLevel Level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		uint32_t CommandBufferCount = 0;
	};

	class VulkanCommandPool {
	public:
		VulkanCommandPool(const CommandPoolCreateInfo& createInfo);
		~VulkanCommandPool() = default;

		static Ref<VulkanCommandPool> Create(const CommandPoolCreateInfo& createInfo);

		inline VkCommandBuffer GetCommandBuffer(uint32_t index) { return m_CommandBuffers[index]; }
		inline size_t GetCommandBufferSize() { return m_CommandBuffers.size(); }

		void Recreate();
		void Destroy();
	private:
		VkCommandBuffer BeginSingleTimeCommand();
		void EndSingleTimeCommand(VkCommandBuffer commandBuffer);

		void Allocate();

		CommandPoolCreateInfo m_CreateInfo;
		VkCommandPool m_CommandPool = VK_NULL_HANDLE;
		std::vector<VkCommandBuffer> m_CommandBuffers;

		friend class CommandQueue; //for singletime commands
	};
}

