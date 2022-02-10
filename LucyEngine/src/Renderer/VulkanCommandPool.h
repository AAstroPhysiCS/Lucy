#pragma once

#include "vulkan/vulkan.h"

namespace Lucy {

	class VulkanPipeline;

	struct CommandPoolSpecs {
		VkCommandPoolCreateFlags PoolFlags = 0;
		VkCommandBufferLevel Level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		uint32_t CommandBufferCount = 0;
	};

	class VulkanCommandPool {
	public:
		VulkanCommandPool(CommandPoolSpecs specs);
		~VulkanCommandPool() = default;

		static RefLucy<VulkanCommandPool> Create(CommandPoolSpecs specs);

		inline VkCommandBuffer GetCommandBuffer(uint32_t index) { return m_CommandBuffers[index]; }
		inline size_t GetCommandBufferSize() { return m_CommandBuffers.size(); }

		void Execute(RefLucy<VulkanPipeline>& pipeline);
		void Recreate();
		void Destroy();
	private:
		void BeginRecording(RefLucy<VulkanPipeline>& pipeline);
		void EndRecording(RefLucy<VulkanPipeline>& pipeline);
		void DirectCopyBuffer(VkBuffer& stagingBuffer, VkBuffer& buffer, VkDeviceSize size);
		void Allocate();

		CommandPoolSpecs m_Specs;
		VkCommandPool m_CommandPool = VK_NULL_HANDLE;
		std::vector<VkCommandBuffer> m_CommandBuffers;

		friend class VulkanRenderer;
	};
}
