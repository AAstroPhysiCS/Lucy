#pragma once

#include <map>

#include "RenderCommandList.h"

#include "Renderer/ExecutionBatch.h"

namespace Lucy {

	enum class TargetQueueFamily : uint8_t;

	using RenderCommandFunc = std::function<void(Ref<RenderDevice>&)>;
	using RenderSubmitFunc = std::function<void(RenderCommandList&)>;

	struct RenderSubmitInfo {
		ExecutionBatch Batch;
		std::vector<RenderSubmitFunc> SubmitFuncs;
	};

	struct RenderCommandQueueCreateInfo {
		Ref<RenderDevice> RenderDevice = nullptr;
		size_t MaxFramesInFlight = 0;
	};

	struct RenderCommandQueueMetricsOutput final {
		double Time;
		std::unordered_map<std::string, double> TimeOfPasses;
	};

	struct ExecutionBatch;

	using RenderSubmitQueue = std::map<ExecutionBatchID, RenderSubmitInfo>;

	class RenderCommandQueue {	
	public:
		RenderCommandQueue(const RenderCommandQueueCreateInfo& createInfo);
		virtual ~RenderCommandQueue() = default;

		RenderCommandQueue(const RenderCommandQueue& other) = delete;
		RenderCommandQueue(RenderCommandQueue&& other) noexcept = delete;
		RenderCommandQueue& operator=(const RenderCommandQueue& other) = delete;
		RenderCommandQueue& operator=(RenderCommandQueue&& other) noexcept = delete;

		void operator+=(RenderCommandFunc&& func);
		void operator+=(RenderSubmitInfo&& info);

		std::vector<RenderCommandList>& GetCommandLists(TargetQueueFamily family);
		inline const std::vector<RenderCommandList>& GetCommandLists(TargetQueueFamily family) const {
			return const_cast<std::vector<RenderCommandList>&>(std::as_const(*this).GetCommandLists(family));
		}

		inline RenderSubmitQueue& GetRenderSubmitQueue() { return m_RenderSubmitQueue; }
		inline const RenderSubmitQueue& GetRenderSubmitQueue() const { return m_RenderSubmitQueue; }

		RenderCommandList& GetNextAvailableCommandList(uint32_t frameIndex, TargetQueueFamily family);
		
		void AllocateCommandLists(const RenderSubmitQueue& submitQueue);
		void ResetFrameSlotRecordersIfCompleted(uint32_t frameIndex, TargetQueueFamily family);

		void Init();
		void RecreateForQueue(TargetQueueFamily family);
		void Recreate();
		void FlushCommandQueue();
		void ClearSubmitQueue();
		void Destroy();
	private:
		RenderCommandQueueCreateInfo m_CreateInfo;
		uint64_t m_BeginTimestampIndex = 0uLL, m_EndTimestampIndex = 0uLL;

		std::map<TargetQueueFamily, std::vector<RenderCommandList>> m_CommandLists;

		std::vector<RenderCommandFunc> m_RenderCommandQueue;
		RenderSubmitQueue m_RenderSubmitQueue;

		inline static std::mutex s_Mutex;
	};
}