#pragma once

#include "Core/Base.h"
#include "DrawCommand.h"

#include "CommandPool.h"

namespace Lucy {

	class Pipeline;

	//for later
	#define MAX_THREAD_COUNT std::thread::hardware_concurrency()

	struct CommandElement {
		Ref<Pipeline> Pipeline = nullptr;
		std::vector<Ref<DrawCommand>> Arguments;

		//first parameter is always the command buffer handle
		//Vulkan: VkCommandBuffer
		RecordFunc<void*, Ref<DrawCommand>> RecordFunc;
	};

	class CommandQueue {
	public:
		CommandQueue() = default;
		virtual ~CommandQueue() = default;

		static Ref<CommandQueue> Create();

		//removing (assignment) constructors for safety
		CommandQueue(const CommandQueue& other) = delete;
		CommandQueue(CommandQueue&& other) noexcept = delete;
		CommandQueue& operator=(const CommandQueue& other) = delete;
		CommandQueue& operator=(CommandQueue&& other) noexcept = delete;

		void Init();
		virtual void Execute() = 0;

		void Enqueue(const CommandElement& element);
		void Recreate();
		void Clear();
		void Free();

		inline size_t GetQueueSize() const { return m_Buffer.size(); }
	protected:
		std::vector<CommandElement> m_Buffer;
		Ref<CommandPool> m_CommandPool = nullptr;
	};
}