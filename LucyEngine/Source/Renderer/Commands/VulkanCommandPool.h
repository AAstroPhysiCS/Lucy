#pragma once

#include "vulkan/vulkan.h"

#include "CommandPool.h"

namespace Lucy {

	class VulkanCommandPool : public CommandPool {
	public:
		VulkanCommandPool(const CommandPoolCreateInfo& createInfo);
		virtual ~VulkanCommandPool() = default;

		VulkanCommandPool(const VulkanCommandPool&) = delete;
		VulkanCommandPool& operator=(const VulkanCommandPool&) = delete;
		VulkanCommandPool(VulkanCommandPool&&) = delete;
		VulkanCommandPool& operator=(VulkanCommandPool&&) = delete;

		inline void* GetCommandBuffer(uint32_t frameIndex) final override { return m_CommandBuffers.at(frameIndex); }
		inline const std::vector<VkCommandBuffer>& GetCommandBuffers() { return m_CommandBuffers; }

		void Destroy();
		void Reset() final override;
		void Recreate() final override;
		void ResetCommandBuffer(uint32_t frameIndex) final override;
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

		VulkanTransientCommandPool(const VulkanTransientCommandPool&) = delete;
		VulkanTransientCommandPool& operator=(const VulkanTransientCommandPool&) = delete;
		VulkanTransientCommandPool(VulkanTransientCommandPool&&) = delete;
		VulkanTransientCommandPool& operator=(VulkanTransientCommandPool&&) = delete;

		VkCommandBuffer BeginSingleTimeCommand(VkDevice logicalDevice);
		void EndSingleTimeCommand();
		void Destroy();

		inline VkCommandBuffer GetTransientCommandBuffer() const { return m_CommandBuffers[m_CommandBuffers.size() - 1]; }
	};
}

