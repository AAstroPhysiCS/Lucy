#include "lypch.h"

#include "CommandQueue.h"
#include "Renderer/Device/VulkanRenderDevice.h"

namespace Lucy {

	//TODO: for now, we will select the 0. index command list and execute only that (in the future, multithread this with JobSystem)
	static inline constexpr const uint32_t commandListIndex = 0;

	CommandQueue::CommandQueue(const CommandQueueCreateInfo& createInfo)
		: m_CreateInfo(createInfo) {
		LUCY_ASSERT(m_CreateInfo.CommandListParallelCount == 1, "Multithreaded command list generation will be supported in the future!");
	}

	void CommandQueue::Init() {
		for (uint32_t i = 0; i < m_CreateInfo.CommandListParallelCount; i++)
			m_CommandLists.emplace_back(m_CreateInfo.RenderDevice);
	}

	void CommandQueue::Recreate() {
		LUCY_PROFILE_NEW_EVENT("CommandQueue::Recreate");
		for (auto& cmdList : m_CommandLists)
			cmdList.Recreate();
	}

	void CommandQueue::FlushCommandQueue() {
		LUCY_PROFILE_NEW_EVENT("CommandQueue::FlushCommandQueue");
		/*	For it to support, nested command/submit function lambdas
			Nested lambda functions are being run in the second iteration.
		*/
		size_t oldCommandSize = m_RenderCommandQueue.size();
		for (size_t i = 0; i < oldCommandSize; i++)
			m_RenderCommandQueue[i](m_CreateInfo.RenderDevice);

		m_RenderCommandQueue.erase(m_RenderCommandQueue.begin(), m_RenderCommandQueue.begin() + oldCommandSize);
	}

	void CommandQueue::FlushSubmitQueue(CommandQueueMetricsOutput& output) {
		LUCY_PROFILE_NEW_EVENT("CommandQueue::FlushSubmitQueue");

		const auto& device = m_CreateInfo.RenderDevice;
		auto& cmdList = m_CommandLists[commandListIndex];
		const auto& primaryCommandPool = cmdList.GetPrimaryCommandPool();

		device->BeginCommandBuffer(primaryCommandPool);
		device->ResetTimestampQuery(primaryCommandPool);
		device->ResetPipelineQuery(primaryCommandPool);
		m_BeginTimestampIndex = device->BeginTimestamp(primaryCommandPool);

		size_t oldSubmitCommandSize = m_RenderSubmitQueue.size();
		for (size_t i = 0; i < oldSubmitCommandSize; i++)
			m_RenderSubmitQueue[i](cmdList);

		m_EndTimestampIndex = device->EndTimestamp(primaryCommandPool);
		auto renderTimes = device->GetQueryResults(RenderDeviceQueryType::Timestamp);
		device->EndCommandBuffer(primaryCommandPool);

		m_RenderSubmitQueue.erase(m_RenderSubmitQueue.begin(), m_RenderSubmitQueue.begin() + oldSubmitCommandSize);

		if (Renderer::GetRenderArchitecture() == RenderArchitecture::Vulkan) {
			double timestampPeriod = device->As<VulkanRenderDevice>()->GetTimestampPeriod();
			for (const auto& [name, cmd] : cmdList.m_RenderCommands) {
				double renderTime = cmd.GetRenderTime(renderTimes) * timestampPeriod / 1000000.0;
				const auto& [it, success] = output.RenderTimeOfPasses.try_emplace(name, renderTime);
				if (!success)
					output.RenderTimeOfPasses.at(name) = renderTime;
			}

			output.RenderTime = (double)(renderTimes[m_EndTimestampIndex] - renderTimes[m_BeginTimestampIndex]) * timestampPeriod / 1000000.0;
		}
	}

	void CommandQueue::Clear() {
		m_RenderSubmitQueue.clear();
		m_RenderCommandQueue.clear();
	}

	void CommandQueue::Free() {
		Clear();
		for (auto& cmdList : m_CommandLists)
			cmdList.Destroy();
	}
}