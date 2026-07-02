#include "lypch.h"

#include "RenderCommandQueue.h"

#include "Renderer/Device/VulkanRenderDevice.h"

#include "Renderer/ExecutionBatch.h"

namespace Lucy {

	RenderCommandQueue::RenderCommandQueue(const RenderCommandQueueCreateInfo& createInfo)
		: m_CreateInfo(createInfo) {
	}

	void RenderCommandQueue::operator+=(RenderCommandFunc&& func) {
		std::unique_lock lock(s_Mutex);
		m_RenderCommandQueue.emplace_back(std::move(func));
	}

	void RenderCommandQueue::operator+=(RenderSubmitInfo&& info) {
		std::unique_lock lock(s_Mutex);
		m_RenderSubmitQueue.try_emplace(info.Batch.ID, std::move(info));
	}

	std::vector<RenderCommandList>& RenderCommandQueue::GetCommandLists(TargetQueueFamily family) {
		return m_CommandLists[family];
	}

	void RenderCommandQueue::Init() {
		
	}

	void RenderCommandQueue::RecreateForQueue(TargetQueueFamily family) {
		auto& recorder = m_CommandLists[family];

		for (auto& [family, cmdLists] : m_CommandLists) {
			for (auto& cmdList : cmdLists)
				cmdList.Recreate();
		}
	}

	void RenderCommandQueue::Recreate() {
		LUCY_PROFILE_NEW_EVENT("RenderCommandQueue::Recreate");
		for (auto& [family, cmdLists] : m_CommandLists) {
			for (auto& cmdList : cmdLists)
				cmdList.Recreate();
		}
	}

	void RenderCommandQueue::ResetFrameSlotRecordersIfCompleted(uint32_t frameIndex, TargetQueueFamily family) {
		auto& cmdLists = m_CommandLists[family];
		for (auto& cmdList : cmdLists)
			cmdList.ResetRenderCommand(frameIndex);
	}

	void RenderCommandQueue::FlushCommandQueue() {
		LUCY_PROFILE_NEW_EVENT("RenderCommandQueue::FlushCommandQueue");
		/*	For it to support, nested command/submit function lambdas
			Nested lambda functions are being run in the second iteration.
		*/
		if (m_RenderCommandQueue.empty())
			return;

		size_t oldCommandSize = m_RenderCommandQueue.size();
		for (size_t i = 0; i < oldCommandSize; i++)
			m_RenderCommandQueue[i](m_CreateInfo.RenderDevice);

		m_RenderCommandQueue.erase(m_RenderCommandQueue.begin(), m_RenderCommandQueue.begin() + oldCommandSize);
	}

	RenderCommandList& RenderCommandQueue::GetNextAvailableCommandList(uint32_t frameIndex, TargetQueueFamily family) {
		auto& cmdLists = GetCommandLists(family);

		for (auto& cmdList : cmdLists) {
			if (cmdList.IsCurrentFrameSlotAvailable(frameIndex))
				return cmdList;
		}

		LUCY_ASSERT(false, "No available command list found for current frame slot!");
	}

	void RenderCommandQueue::AllocateCommandLists(const RenderSubmitQueue& submitQueue) {
		std::array<size_t, static_cast<size_t>(TargetQueueFamily::Count)> batchIndexWithinFamily{};

		for (const auto& [id, info] : submitQueue) {
			const auto& vkBatch = info.Batch.AsVulkanBatch();
			auto& cmdLists = GetCommandLists(vkBatch.QueueFamily);

			auto index = static_cast<uint8_t>(vkBatch.QueueFamily);
			batchIndexWithinFamily[index]++;

			while (cmdLists.size() < batchIndexWithinFamily[index]) {
				cmdLists.emplace_back(RenderCommandListCreateInfo{
					.RenderDevice = m_CreateInfo.RenderDevice,
					.TargetQueueFamily = vkBatch.QueueFamily
				});
			}
		}
	}

	void RenderCommandQueue::Clear() {
		m_RenderSubmitQueue.clear();
		m_RenderCommandQueue.clear();
	}

	void RenderCommandQueue::Destroy() {
		Clear();

		for (auto& [family, cmdLists] : m_CommandLists) {
			for (auto& cmdList : cmdLists)
				cmdList.Destroy();
		}
	}
}