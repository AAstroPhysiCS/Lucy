#include "lypch.h"
#include "RenderCommandList.h"
#include "VulkanCommandPool.h"

#include "Renderer/Image/VulkanImage.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	static inline RenderCommand* s_CurrentActiveRenderCommand = nullptr;

	RenderCommandList::RenderCommandList(const RenderCommandListCreateInfo& createInfo) 
		: m_CreateInfo(createInfo) {
		auto commandPoolCreateInfo = CommandPoolCreateInfo{
			.CommandBufferCount = Renderer::GetMaxFramesInFlight(),
			.Level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.PoolFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.RenderDevice = m_CreateInfo.RenderDevice,
			.TargetQueueFamily = m_CreateInfo.TargetQueueFamily
		};

		/*
		* TODO:
		static auto secondaryCommandPoolCreateInfo = VulkanCommandPoolCreateInfo{
			.CommandBufferCount = Renderer::GetMaxFramesInFlight(),
			.Level = VK_COMMAND_BUFFER_LEVEL_SECONDARY,
			.PoolFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
			.RenderDevice = m_CreateInfo.RenderDevice
		};*/

		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan:
				m_PrimaryCommandPool = Memory::CreateRef<VulkanCommandPool>(commandPoolCreateInfo);
				//m_SecondaryCommandPool = Memory::CreateRef<VulkanCommandPool>(secondaryCommandPoolCreateInfo);
				break;
			default:
				LUCY_ASSERT(false, "No suitable API found to create the resource!");
		}

		m_QueryDatas.resize(Renderer::GetMaxFramesInFlight());
	}

	RenderCommand& RenderCommandList::BeginRenderCommand(const std::string& nameOfDraw) {
		LUCY_ASSERT(!s_CurrentActiveRenderCommand, "There is an active ongoing render command that needs to be closed!");
		LUCY_PROFILE_NEW_EVENT("RenderCommandList::BeginRenderCommand");

		if (!m_RenderCommands.empty() && m_RenderCommands.back().GetDebugName() == nameOfDraw) {
			auto& cmd = m_RenderCommands.back();
			cmd.BeginDebugMarker();
			s_CurrentActiveRenderCommand = &cmd;
			return cmd;
		}

		m_RenderCommands.emplace_back(nameOfDraw, m_CreateInfo.RenderDevice, m_PrimaryCommandPool);

		RenderCommand& cmd = m_RenderCommands.back();
		cmd.BeginTimestamp();
		cmd.BeginDebugMarker();

		s_CurrentActiveRenderCommand = &cmd;
		return cmd;
	}

	void RenderCommandList::EndRenderCommand() {
		LUCY_ASSERT(s_CurrentActiveRenderCommand, "There isn't any active ongoing render command right now!");
		LUCY_PROFILE_NEW_EVENT("RenderCommandList::EndRenderCommand");

		s_CurrentActiveRenderCommand->EndPipelineStatistics();
		s_CurrentActiveRenderCommand->EndDebugMarker();
		s_CurrentActiveRenderCommand->EndTimestamp();

		auto& queryData = m_QueryDatas[Renderer::GetCurrentFrameIndex()];
		queryData.TimestampScopes.emplace_back(s_CurrentActiveRenderCommand->GetDebugName(), s_CurrentActiveRenderCommand->m_BeginTimestampIndex, s_CurrentActiveRenderCommand->m_EndTimestampIndex);
		queryData.PipelineScopes.emplace_back(s_CurrentActiveRenderCommand->GetDebugName(), s_CurrentActiveRenderCommand->m_BeginPipelineQueryIndex);

		s_CurrentActiveRenderCommand = nullptr;
	}

	bool RenderCommandList::IsCurrentFrameSlotAvailable(uint32_t frameIndex) const {
		auto pool = m_PrimaryCommandPool->As<VulkanCommandPool>();
		return pool->GetState(frameIndex) == CommandBufferSlotState::Ready;
	}

	bool RenderCommandList::IsCurrentFrameSlotRecorded(uint32_t frameIndex) const {
		auto pool = m_PrimaryCommandPool->As<VulkanCommandPool>();
		return pool->GetState(frameIndex) == CommandBufferSlotState::Recorded;
	}

	void RenderCommandList::Reset() {
		LUCY_PROFILE_NEW_EVENT("RenderCommandList::Reset");
		m_RenderCommands.clear();
		m_PrimaryCommandPool->Reset();
	}

	void RenderCommandList::ResetRenderCommand(uint32_t frameIndex) {
		LUCY_PROFILE_NEW_EVENT("RenderCommandList::ResetRenderCommand");
		m_PrimaryCommandPool->ResetCommandBuffer(frameIndex);
		m_QueryDatas[frameIndex].TimestampScopes.clear();
		m_QueryDatas[frameIndex].PipelineScopes.clear();
	}

	void RenderCommandList::Recreate() {
		LUCY_PROFILE_NEW_EVENT("RenderCommandList::Recreate");
		m_RenderCommands.clear();
		m_PrimaryCommandPool->Recreate();
		//m_SecondaryCommandPool->Recreate();
	}

	void RenderCommandList::Destroy() {
		LUCY_PROFILE_NEW_EVENT("RenderCommandList::Destroy");
		m_RenderCommands.clear();
		m_PrimaryCommandPool->Destroy();
		//m_SecondaryCommandPool->Destroy();
	}
}