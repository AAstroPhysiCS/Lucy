#pragma once

#include "Renderer/Memory/Memory.h"

#include "RenderCommand.h"
#include "Renderer/Device/RenderDevice.h"

#include "CommandPool.h"
#include "Renderer/Memory/Buffer/Buffer.h"

namespace Lucy {

	struct RenderCommandListCreateInfo {
		Ref<RenderDevice> RenderDevice = nullptr;
		TargetQueueFamily TargetQueueFamily = TargetQueueFamily::Count;
	};

	struct TimestampQueryScope {
		std::string NameOfDraw = "Unknown Draw";
		uint32_t BeginQueryIndex = 0;
		uint32_t EndQueryIndex = 0;
	};

	struct PipelineQueryScope {
		std::string NameOfDraw = "Unknown Draw";
		uint32_t QueryIndex = 0;
	};

	struct RenderCommandListQueryData {
		std::vector<TimestampQueryScope> TimestampScopes;
		std::vector<PipelineQueryScope> PipelineScopes;
	};

	class RenderCommandList final {
	public:
		RenderCommandList(const RenderCommandListCreateInfo& createInfo);
		~RenderCommandList() = default;

		RenderCommandList(const RenderCommandList&) = delete;
		RenderCommandList& operator=(const RenderCommandList&) = delete;
		RenderCommandList(RenderCommandList&&) noexcept = default;
		RenderCommandList& operator=(RenderCommandList&&) noexcept = default;

		RenderCommand BeginRenderCommand();
		void EndRenderCommand(const std::string& nameOfDraw, RenderCommand& cmd);

		bool IsCurrentFrameSlotAvailable(uint32_t frameIndex) const;
		bool IsCurrentFrameSlotRecorded(uint32_t frameIndex) const;

		explicit operator bool() const { return !m_RenderCommands.empty(); }

		TargetQueueFamily GetTargetQueueFamily() const { return m_CreateInfo.TargetQueueFamily; }
		Ref<CommandPool> GetPrimaryCommandPool() const { return m_PrimaryCommandPool; }
		const RenderCommandListQueryData& GetQueryData(uint32_t frameIndex) const { return m_QueryDatas[frameIndex]; }
	private:
		void Reset();
		void ResetRenderCommand(uint32_t frameIndex);
		void Recreate();
		void Destroy();
		
		std::vector<RenderCommand> m_RenderCommands;

		RenderCommandListCreateInfo m_CreateInfo;
		Ref<CommandPool> m_PrimaryCommandPool = nullptr;
		//Ref<CommandPool> m_SecondaryCommandPool = nullptr;

		std::vector<RenderCommandListQueryData> m_QueryDatas;

		friend class RenderCommandQueue; //for Destroy/Recreate
		friend class VulkanRenderer; //for ResetRenderCommand
		friend struct ImGuiVulkanImpl; //for RenderDevice
	};
}