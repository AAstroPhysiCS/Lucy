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

	class RenderCommandList final {
	public:
		RenderCommandList(const RenderCommandListCreateInfo& createInfo);
		~RenderCommandList() = default;

		RenderCommandList(const RenderCommandList&) = delete;
		RenderCommandList& operator=(const RenderCommandList&) = delete;
		RenderCommandList(RenderCommandList&&) noexcept = default;
		RenderCommandList& operator=(RenderCommandList&&) noexcept = default;

		RenderCommand& BeginRenderCommand(const std::string& nameOfDraw);
		void EndRenderCommand() const;

		bool IsCurrentFrameSlotAvailable(uint32_t frameIndex) const;
		bool IsCurrentFrameSlotRecorded(uint32_t frameIndex) const;

		explicit operator bool() const { return !m_RenderCommands.empty(); }

		inline TargetQueueFamily GetTargetQueueFamily() const { return m_CreateInfo.TargetQueueFamily; }
		inline Ref<CommandPool> GetPrimaryCommandPool() const { return m_PrimaryCommandPool; }
	private:
		void Reset();
		void ResetRenderCommand(uint32_t frameIndex);
		void Recreate();
		void Destroy();
		
		std::vector<RenderCommand> m_RenderCommands;

		RenderCommandListCreateInfo m_CreateInfo;
		Ref<CommandPool> m_PrimaryCommandPool = nullptr;
		//Ref<CommandPool> m_SecondaryCommandPool = nullptr;

		friend class RenderCommandQueue; //for Destroy/Recreate
		friend class VulkanRenderer; //for ResetRenderCommand
	};
}