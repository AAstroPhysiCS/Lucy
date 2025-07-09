#include "lypch.h"
#include "RenderCommandList.h"
#include "VulkanCommandPool.h"

#include "Renderer/Image/VulkanImage.h"

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
	}

	RenderCommand& RenderCommandList::BeginRenderCommand(const std::string& nameOfDraw) {
		LUCY_ASSERT(!s_CurrentActiveRenderCommand, "There is an active ongoing render command that needs to be closed!");
		LUCY_PROFILE_NEW_EVENT(std::format("RenderCommandList::BeginRenderCommand {}", nameOfDraw).c_str());

		if (!m_RenderCommands.contains(nameOfDraw))
			m_RenderCommands.try_emplace(nameOfDraw, nameOfDraw, m_CreateInfo.RenderDevice, m_PrimaryCommandPool);

		RenderCommand& cmd = m_RenderCommands.at(nameOfDraw);
		cmd.BeginTimestamp();
		cmd.BeginDebugMarker();

		s_CurrentActiveRenderCommand = &cmd;
		return cmd;
	}

	void RenderCommandList::EndRenderCommand() const {
		LUCY_ASSERT(s_CurrentActiveRenderCommand, "There isn't any active ongoing render command right now!");
		LUCY_PROFILE_NEW_EVENT(std::format("RenderCommandList::EndRenderCommand{}", s_CurrentActiveRenderCommand->GetDebugName()).c_str());

		s_CurrentActiveRenderCommand->EndPipelineStatistics();
		s_CurrentActiveRenderCommand->EndDebugMarker();
		s_CurrentActiveRenderCommand->EndTimestamp();

		s_CurrentActiveRenderCommand = nullptr;
	}

	void RenderCommandList::Recreate() {
		m_RenderCommands.clear();
		m_PrimaryCommandPool->Recreate();
		//m_SecondaryCommandPool->Recreate();
	}

	void RenderCommandList::Destroy() {
		m_RenderCommands.clear();
		m_PrimaryCommandPool->Destroy();
		//m_SecondaryCommandPool->Destroy();
	}
}