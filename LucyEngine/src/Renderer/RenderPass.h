#pragma once

#include "../Core/Base.h"

#include "vulkan/vulkan.h"
#include "Buffer/OpenGL/OpenGLFrameBuffer.h"

namespace Lucy {

	struct ClearColor {
		float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
	};

	struct VulkanAttachmentDescriptor {
		VkAttachmentLoadOp LoadOp;
		VkAttachmentStoreOp StoreOp;
		VkAttachmentLoadOp StencilLoadOp;
		VkAttachmentStoreOp StencilStoreOp;
		VkImageLayout InitialLayout;
		VkImageLayout FinalLayout;
	};

	struct RenderPassSpecification {
		ClearColor ClearColor;
		std::vector<VkAttachmentReference> AttachmentReferences;
		VulkanAttachmentDescriptor Descriptor;
	};

	struct RenderPassBeginInfo {
		RefLucy<OpenGLFrameBuffer> OpenGLFrameBuffer = nullptr;
		VkFramebuffer VulkanFrameBuffer = nullptr;
		VkCommandBuffer CommandBuffer = nullptr;
	};

	struct RenderPassEndInfo {
		RefLucy<OpenGLFrameBuffer> OpenGLFrameBuffer = nullptr;
		VkFramebuffer VulkanFrameBuffer = nullptr;
		VkCommandBuffer CommandBuffer = nullptr;
	};

	class RenderPass {
	public:
		static RefLucy<RenderPass> Create(const RenderPassSpecification& specs);

		RenderPass(const RenderPassSpecification& specs);

		virtual void Begin(RenderPassBeginInfo& info) = 0;
		virtual void End(RenderPassEndInfo& info) = 0;

		inline ClearColor GetClearColor() { return m_Specs.ClearColor; }
	protected:
		RenderPassSpecification m_Specs;
	};
}

