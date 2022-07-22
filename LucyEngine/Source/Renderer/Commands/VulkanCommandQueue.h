#pragma once

#include "CommandQueue.h"

namespace Lucy {

	class VulkanCommandQueue : public CommandQueue {
	public:
		VulkanCommandQueue() = default;
		virtual ~VulkanCommandQueue() = default;

		//removing (assignment) constructors for safety
		VulkanCommandQueue(const VulkanCommandQueue& other) = delete;
		VulkanCommandQueue(VulkanCommandQueue&& other) noexcept = delete;
		VulkanCommandQueue& operator=(const VulkanCommandQueue& other) = delete;
		VulkanCommandQueue& operator=(VulkanCommandQueue&& other) noexcept = delete;

		VkCommandBuffer BeginSingleTimeCommand() const;
		void EndSingleTimeCommand(VkCommandBuffer commandBuffer) const;
		VkCommandBuffer GetCurrentCommandBuffer() const;

		void Execute() override;
	};
}