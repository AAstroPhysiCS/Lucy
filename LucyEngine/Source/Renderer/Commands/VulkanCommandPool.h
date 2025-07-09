#pragma once

#include "vulkan/vulkan.h"

#include "CommandPool.h"

namespace Lucy {

	class VulkanCommandPool : public CommandPool {
	public:
		VulkanCommandPool(const CommandPoolCreateInfo& createInfo);
		virtual ~VulkanCommandPool() = default;

		inline void* GetCurrentFrameCommandBuffer() { return m_CommandBuffers[Renderer::GetCurrentFrameIndex()]; }

		void Destroy();
		void Recreate() final override;
	protected:
		void FreeCommandBuffers(uint32_t commandBufferCount, size_t commandBufferStartIndex);

		inline VkCommandBuffer GetCommandBuffer(size_t index) { return m_CommandBuffers[index]; }
		inline size_t GetCommandBufferSize() const { return m_CommandBuffers.size(); }

		VkCommandPool m_CommandPool = VK_NULL_HANDLE;
		std::vector<VkCommandBuffer> m_CommandBuffers;
	};

	class VulkanTransientCommandPool final : private VulkanCommandPool {
	public:
		VulkanTransientCommandPool(const Ref<VulkanRenderDevice>& vulkanDevice);
		virtual ~VulkanTransientCommandPool() = default;

		VkCommandBuffer BeginSingleTimeCommand(VkDevice logicalDevice);
		void EndSingleTimeCommand();
		void Destroy();

		inline VkCommandBuffer GetTransientCommandBuffer() const { return m_CommandBuffers[m_CommandBuffers.size() - 1]; }
	};
}

