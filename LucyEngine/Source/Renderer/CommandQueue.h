#pragma once

#include <vector>

#include "vulkan/vulkan.h"
#include "Core/Base.h"
#include "DrawCommand.h"

namespace Lucy {

	class VulkanCommandPool;
	class Pipeline;

	//for later
	#define MAX_THREAD_COUNT std::thread::hardware_concurrency()

	struct CommandElement {
		Ref<Pipeline> Pipeline = nullptr;
		std::vector<Ref<DrawCommand>> Arguments;

		RecordFunc<Ref<DrawCommand>> RecordFunc;
	};

	class CommandQueue {
	public:
		CommandQueue() = default;
		~CommandQueue() = default;

		//removing (assignment) constructors for safety
		CommandQueue(const CommandQueue& other) = delete;
		CommandQueue(CommandQueue&& other) noexcept = delete;
		CommandQueue& operator=(const CommandQueue& other) = delete;
		CommandQueue& operator=(CommandQueue&& other) noexcept = delete;

		void Init();
		void Enqueue(const CommandElement& element);
		void Execute();
		void Recreate();
		void Clear();
		void Free();

		VkCommandBuffer GetCurrentCommandBuffer() const;
		inline size_t GetQueueSize() const { return m_Buffer.size(); }

		VkCommandBuffer BeginSingleTimeCommand() const;
		void EndSingleTimeCommand(VkCommandBuffer commandBuffer) const;
	private:
		std::vector<CommandElement> m_Buffer;
		Ref<VulkanCommandPool> m_CommandPool;
	};
}