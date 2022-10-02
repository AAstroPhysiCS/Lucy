#pragma once

#include "CommandResources.h"

#include "CommandPool.h"

namespace Lucy {

	class Fence;
	class Semaphore;

	class ContextPipeline;
	class GraphicsPipeline;

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

		virtual void Init() = 0;
		virtual void Execute() = 0;

		virtual void SubmitWorkToGPU(void* queueHandle, const Fence& currentFrameFence, const Semaphore& currentFrameWaitSemaphore, const Semaphore& currentFrameSignalSemaphore) const = 0;
		virtual void SubmitWorkToGPU(void* queueHandle, uint32_t commandBufferCount, void* commandBufferHandles) const = 0;

		CommandResourceHandle CreateCommandResource(Ref<ContextPipeline> pipeline, CommandFunc&& func);
		CommandResourceHandle CreateChildCommandResource(CommandResourceHandle parentResourceHandle, Ref<GraphicsPipeline> childPipeline, CommandFunc&& func);
		void DeleteCommandResource(CommandResourceHandle commandHandle);
		void EnqueueCommand(CommandResourceHandle resourceHandle, const Ref<RenderCommand>& command);
		void Recreate();
		void Clear();
		void Free();

		inline size_t GetQueueSize() const { return m_CommandResourceMap.size(); }
	protected:
		std::map<CommandResourceHandle, CommandResource> m_CommandResourceMap;
		Ref<CommandPool> m_CommandPool = nullptr;
	};
}