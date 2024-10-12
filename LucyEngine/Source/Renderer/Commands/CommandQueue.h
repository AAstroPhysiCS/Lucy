#pragma once

#include "RenderCommandList.h"

namespace Lucy {

	using RenderCommandFunc = std::function<void(Ref<RenderDevice>&)>;
	using RenderSubmitFunc = std::function<void(RenderCommandList&)>;

	struct CommandQueueCreateInfo {
		//TODO: Expand this further and implement multithreaded command buffer submissions.
		size_t CommandListParallelCount = 1; // if 1, then the submission is being only done in the main thread.
		Ref<RenderDevice> RenderDevice = nullptr;
	};

	struct CommandQueueMetricsOutput final {
		double RenderTime;
		std::unordered_map<std::string, double> RenderTimeOfPasses;
	};

	class CommandQueue final {
	public:
		CommandQueue(const CommandQueueCreateInfo& createInfo);
		~CommandQueue() = default;

		CommandQueue(const CommandQueue& other) = delete;
		CommandQueue(CommandQueue&& other) noexcept = delete;
		CommandQueue& operator=(const CommandQueue& other) = delete;
		CommandQueue& operator=(CommandQueue&& other) noexcept = delete;

		inline void operator+=(RenderCommandFunc&& func) { m_RenderCommandQueue.emplace_back(std::move(func)); }
		inline void operator+=(RenderSubmitFunc&& func) { m_RenderSubmitQueue.emplace_back(std::move(func)); }

		inline const std::vector<RenderCommandList>& GetCommandLists() const { return m_CommandLists; }

		void Init();
		void Recreate();
		void FlushCommandQueue();
		void FlushSubmitQueue(CommandQueueMetricsOutput& output);
		void Clear();
		void Free();
	private:
		CommandQueueCreateInfo m_CreateInfo;
		uint64_t m_BeginTimestampIndex = 0uLL, m_EndTimestampIndex = 0uLL;

		std::vector<RenderCommandList> m_CommandLists;

		std::vector<RenderCommandFunc> m_RenderCommandQueue;
		std::vector<RenderSubmitFunc> m_RenderSubmitQueue;
	};
}