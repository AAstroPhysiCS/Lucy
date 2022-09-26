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
		void EndSingleTimeCommand() const;
		VkCommandBuffer GetCurrentCommandBuffer() const;

		void Init() final override;
		void Execute() final override;
		void SubmitWorkToGPU(void* queueHandle, const Fence& currentFrameFence, const Semaphore& currentFrameWaitSemaphore, const Semaphore& currentFrameSignalSemaphore) const final override;
		void SubmitWorkToGPU(void* queueHandle, uint32_t commandBufferCount, void* commandBufferHandles) const final override;
	};
}