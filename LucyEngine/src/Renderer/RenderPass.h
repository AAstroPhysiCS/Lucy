#pragma once

#include "../Core/Base.h"

#include "vulkan/vulkan.h"

namespace Lucy {

	struct ClearColor {
		float r = 0, g = 0, b = 0, a = 0;
	};

	struct RenderPassSpecification {
		ClearColor ClearColor;
	};

	//Vulkan only
	struct RenderPassBeginInfo {
	private:
		VkCommandBuffer CommandBuffer;
		VkFramebuffer FrameBuffer;

		friend class VulkanRenderPass;
		friend class VulkanRenderCommand;
	};

	//Vulkan only
	struct RenderPassEndInfo {
	private:
		VkCommandBuffer CommandBuffer;

		friend class VulkanRenderPass;
		friend class VulkanRenderCommand;
	};

	class RenderPass {
	public:
		static RefLucy<RenderPass> Create(RenderPassSpecification& specs);

		RenderPass(RenderPassSpecification& specs);
		
		virtual void Begin(RenderPassBeginInfo& info) = 0;
		virtual void End(RenderPassEndInfo& info) = 0;
		
		inline ClearColor GetClearColor() { return m_Specs.ClearColor; }
	protected:
		RenderPassSpecification m_Specs;
	};
}

