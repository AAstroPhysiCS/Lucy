#pragma once

#include "RenderCommand.h"

#include "CommandPool.h"
#include "Renderer/Memory/Buffer/Buffer.h"

namespace Lucy {

	struct RenderCommandListCreateInfo {
		Ref<RenderDevice> RenderDevice = nullptr;
		TargetQueueFamily TargetQueueFamily;
	};

	class RenderCommandList final {
	public:
		RenderCommandList(const RenderCommandListCreateInfo& createInfo);
		~RenderCommandList() = default;

		RenderCommand& BeginRenderCommand(const std::string& nameOfDraw);
		void EndRenderCommand() const;

		explicit operator bool() const { return m_RenderCommands.empty(); }

		inline Ref<CommandPool> GetPrimaryCommandPool() const { return m_PrimaryCommandPool; }
	private:
		void Recreate();
		void Destroy();
		
		std::unordered_map<std::string, RenderCommand> m_RenderCommands;

		RenderCommandListCreateInfo m_CreateInfo;
		Ref<CommandPool> m_PrimaryCommandPool = nullptr;
		//Ref<CommandPool> m_SecondaryCommandPool = nullptr;

		friend class RenderCommandQueue; //for Destroy/Recreate
	};
}