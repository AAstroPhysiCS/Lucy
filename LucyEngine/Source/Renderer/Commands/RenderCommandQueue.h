#pragma once

#include "RenderCommandList.h"

namespace Lucy {

	enum class TargetQueueFamily : uint8_t;

	using RenderCommandFunc = std::function<void(Ref<RenderDevice>&)>;
	using RenderSubmitFunc = std::function<void(RenderCommandList&)>;

	struct RenderCommandQueueCreateInfo {
		//TODO: Expand this further and implement multithreaded command buffer submissions.
		size_t CommandListParallelCount = 1; // if 1, then the submission is being only done in the main thread.
		Ref<RenderDevice> RenderDevice = nullptr;
		TargetQueueFamily TargetQueueFamily;
	};

	struct RenderCommandQueueMetricsOutput final {
		double RenderTime;
		std::unordered_map<std::string, double> RenderTimeOfPasses;
	};

	class RenderCommandQueue final {
	public:
		RenderCommandQueue(const RenderCommandQueueCreateInfo& createInfo);
		~RenderCommandQueue() = default;

		RenderCommandQueue(const RenderCommandQueue& other) = delete;
		RenderCommandQueue(RenderCommandQueue&& other) noexcept = delete;
		RenderCommandQueue& operator=(const RenderCommandQueue& other) = delete;
		RenderCommandQueue& operator=(RenderCommandQueue&& other) noexcept = delete;

		inline void operator+=(RenderCommandFunc&& func) { 
			std::unique_lock lock(s_Mutex);
			m_RenderCommandQueue.emplace_back(std::move(func)); 
		}
		inline void operator+=(RenderSubmitFunc&& func) { 
			std::unique_lock lock(s_Mutex);
			m_RenderSubmitQueue.emplace_back(std::move(func)); 
		}

		inline bool IsEmpty() const { return m_RenderSubmitQueue.empty(); }

		inline const std::vector<RenderCommandList>& GetCommandLists() const { return m_CommandLists; }

		void Init();
		void Recreate();
		void FlushCommandQueue();
		void FlushSubmitQueue(RenderCommandQueueMetricsOutput& output);
		void Clear();
		void Free();
	private:
		RenderCommandQueueCreateInfo m_CreateInfo;
		uint64_t m_BeginTimestampIndex = 0uLL, m_EndTimestampIndex = 0uLL;

		std::vector<RenderCommandList> m_CommandLists;

		std::vector<RenderCommandFunc> m_RenderCommandQueue;
		std::vector<RenderSubmitFunc> m_RenderSubmitQueue;

		inline static std::mutex s_Mutex;
	};
}