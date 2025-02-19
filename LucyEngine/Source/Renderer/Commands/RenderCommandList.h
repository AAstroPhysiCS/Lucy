#pragma once

#include "RenderCommand.h"

#include "CommandPool.h"
#include "Renderer/Memory/Buffer/Buffer.h"

namespace Lucy {

	class RenderCommandList final {
	public:
		RenderCommandList(const Ref<RenderDevice>& renderDevice);
		~RenderCommandList() = default;

		RenderCommand& BeginRenderCommand(const std::string& nameOfDraw);
		void EndRenderCommand() const;

		inline Ref<CommandPool> GetPrimaryCommandPool() const { return m_PrimaryCommandPool; }
	private:
		void Recreate();
		void Destroy();
		
		std::unordered_map<std::string, RenderCommand> m_RenderCommands;

		Ref<RenderDevice> m_RenderDevice = nullptr;
		Ref<CommandPool> m_PrimaryCommandPool = nullptr;
		//Ref<CommandPool> m_SecondaryCommandPool = nullptr;

		friend class CommandQueue; //for Destroy/Recreate
	};
}