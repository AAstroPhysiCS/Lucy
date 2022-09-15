#pragma once

#include "CommandResources.h"

#include "CommandPool.h"

namespace Lucy {

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

		CommandResourceHandle CreateCommandResource(CommandFunc&& func, Ref<GraphicsPipeline> pipeline);
		void DeleteCommandResource(CommandResourceHandle commandHandle);
		void EnqueueCommand(CommandResourceHandle resourceHandle, const Ref<RenderCommand>& command);
		void Recreate();
		void Clear();
		void Free();

		inline size_t GetQueueSize() const { return m_RenderResourceMap.size(); }
	protected:
		std::unordered_map<CommandResourceHandle, RenderCommandResource> m_RenderResourceMap;
		Ref<CommandPool> m_CommandPool = nullptr;
	};
}