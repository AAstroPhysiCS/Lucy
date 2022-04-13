#pragma once

#include "Context/VulkanPipeline.h"

namespace Lucy {

	using VulkanRenderCommandFunc = std::function<void(VkCommandBuffer commandBuffer)>;

	///TODO:
	///RenderCommand can contain VulkanCommandPool -> more suitable for multi - threading
	///for multi-threading, we even can create a another class for "SecondaryRenderCommands" and use it as an abstraction
	
	class RenderCommand {
	public:
		static RenderCommand BeginNew();
		static void End();

		inline static RenderCommand* s_CurrentRenderCommand = nullptr;

		VulkanRenderCommandFunc Func;
		RefLucy<VulkanPipeline> Pipeline = nullptr;

		~RenderCommand() = default;
	private:
		RenderCommand() = default;
	};

	class RenderCommandQueue {
	public:
		RenderCommandQueue() = default;
		~RenderCommandQueue() = default;

		void SubmitToQueue(const RenderCommand& cmd);
		void Clear();

		inline std::vector<RenderCommand> GetCommandQueue() const { return m_CommandQueue; }
		inline std::vector<RenderCommand> GetCommandQueue() { return m_CommandQueue; }
	private:
		std::vector<RenderCommand> m_CommandQueue;
	};
}

