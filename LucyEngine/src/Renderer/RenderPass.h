#pragma once

#include "../Core/Base.h"

#include "vulkan/vulkan.h"

namespace Lucy {

	class OpenGLFrameBuffer;
	class OpenGLRenderPass;

	class VulkanRenderPass;

	struct ClearColor {
		float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
	};

	struct VulkanAttachmentDescriptor {
		VkFormat Format;
		VkAttachmentLoadOp LoadOp;
		VkAttachmentStoreOp StoreOp;
		VkAttachmentLoadOp StencilLoadOp;
		VkAttachmentStoreOp StencilStoreOp;
		VkImageLayout InitialLayout;
		VkImageLayout FinalLayout;
	};

	struct VulkanRHIRenderPassDesc {
		std::vector<VkAttachmentReference> ColorAttachments;
		VulkanAttachmentDescriptor ColorDescriptor;
		bool DepthEnable = false;
	};

	struct RenderPassSpecification {
		ClearColor ClearColor;
		Ref<void> InternalInfo = nullptr;  //to be overriden by different rhi's (OpenGL: none)
	};

	struct RenderPassBeginInfo {
		Ref<OpenGLFrameBuffer> OpenGLFrameBuffer = nullptr;
		VkFramebuffer VulkanFrameBuffer = nullptr;
		VkCommandBuffer CommandBuffer = nullptr;
		uint32_t Width = 0;
		uint32_t Height = 0;
	};

	class RenderPass {
	public:
		static Ref<RenderPass> Create(const RenderPassSpecification& specs);

		virtual void Begin(RenderPassBeginInfo& info) = 0;
		virtual void End() = 0;
		virtual void Recreate() = 0;
		virtual void Destroy() = 0;

		RenderPass(const RenderPassSpecification& specs);
		virtual ~RenderPass() = default;

		inline ClearColor GetClearColor() { return m_Specs.ClearColor; }
	protected:
		RenderPassSpecification m_Specs;
	};
}