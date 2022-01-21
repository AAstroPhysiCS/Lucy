#pragma once

#include "../Core/Base.h"

#include "vulkan/vulkan.h"
#include "Buffer/OpenGL/OpenGLFrameBuffer.h"

namespace Lucy {

	struct ClearColor {
		float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
	};

	struct RenderPassSpecification {
		ClearColor ClearColor;
		std::vector<VkAttachmentReference> AttachmentReferences;
	};
	
	struct RenderPassBeginInfo {
		RefLucy<OpenGLFrameBuffer> OpenGLFrameBuffer = nullptr;
		VkFramebuffer VulkanFrameBuffer;
		VkCommandBuffer CommandBuffer;
	};

	struct RenderPassEndInfo {
		RefLucy<OpenGLFrameBuffer> OpenGLFrameBuffer = nullptr;
		VkFramebuffer FrameBuffer;
		VkCommandBuffer CommandBuffer;
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

