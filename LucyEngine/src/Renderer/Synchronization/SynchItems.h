#pragma once

#include "vulkan/vulkan.h"

namespace Lucy {

	//TODO: VK Synchronization2
	class Semaphore {
	public:
		Semaphore();
		~Semaphore() = default;

		void Destroy();

		inline VkSemaphore& GetSemaphore() { return m_Handle; }
	private:
		VkSemaphore m_Handle;
	};

	class Fence {
	public:
		Fence();
		~Fence() = default;

		void Destroy();

		inline VkFence& GetFence() { return m_Handle; }
	private:
		VkFence m_Handle;
	};
}