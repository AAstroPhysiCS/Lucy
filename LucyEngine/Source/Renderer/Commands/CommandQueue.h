#pragma once

#include "RenderCommandResources.h"

#include "CommandPool.h"

namespace Lucy {

	class Pipeline;

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

		RenderCommandResourceHandle CreateRenderPassResource(RenderCommandFunc&& func, Ref<Pipeline> pipeline);
		void EnqueueRenderCommand(RenderCommandResourceHandle resourceHandle, const Ref<RenderCommand>& command);
		void Recreate();
		void Clear();
		void Free();

		inline size_t GetQueueSize() const { return m_BufferMap.size(); }
	protected:
		std::map<RenderCommandResourceHandle, RenderCommandResource> m_BufferMap;
		Ref<CommandPool> m_CommandPool = nullptr;
	};
}