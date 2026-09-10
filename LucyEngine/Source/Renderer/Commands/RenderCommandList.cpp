#include "lypch.h"
#include "RenderCommandList.h"
#include "VulkanCommandPool.h"

#include "Renderer/Image/VulkanImage.h"

#include "Renderer/Renderer.h"

namespace Lucy {

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

	RenderCommand RenderCommandList::BeginRenderCommand() {
		LUCY_PROFILE_NEW_EVENT("RenderCommandList::BeginRenderCommand");
		RenderCommand cmd = { m_CreateInfo.RenderDevice, m_PrimaryCommandPool };
		cmd.BeginTimestamp();
		return cmd;
	}

	void RenderCommandList::EndRenderCommand(const std::string& nameOfDraw, RenderCommand& cmd) {
		LUCY_ASSERT(&cmd, "There isn't any active ongoing render command right now!");
		LUCY_PROFILE_NEW_EVENT("RenderCommandList::EndRenderCommand");

		cmd.EndPipelineStatistics();
		cmd.EndTimestamp();

		auto& queryData = m_QueryDatas[Renderer::GetCurrentFrameIndex()];
		queryData.TimestampScopes.emplace_back(nameOfDraw, cmd.m_BeginTimestampIndex, cmd.m_EndTimestampIndex);
		queryData.PipelineScopes.emplace_back(nameOfDraw, cmd.m_BeginPipelineQueryIndex);
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