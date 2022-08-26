#pragma once

#include "vulkan/vulkan.h"

namespace Lucy {

	enum class ImageFormat;

	struct ClearColor {
		float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
	};

	struct VulkanAttachmentDescriptor {
		ImageFormat Format;
		VkAttachmentLoadOp LoadOp;
		VkAttachmentStoreOp StoreOp;
		VkAttachmentLoadOp StencilLoadOp;
		VkAttachmentStoreOp StencilStoreOp;
		VkImageLayout InitialLayout;
		VkImageLayout FinalLayout;
	};

	struct VulkanRenderPassInfo {
		std::vector<VkAttachmentReference> ColorAttachments;
		VulkanAttachmentDescriptor ColorDescriptor;
	};

	struct RenderPassCreateInfo {
		bool DepthEnable = false;
		ClearColor ClearColor;
		Ref<void> InternalInfo = nullptr;  //to be overriden by different api's
	};

	struct RenderPassBeginInfo {
		VkFramebuffer VulkanFrameBuffer = nullptr;
		VkCommandBuffer CommandBuffer = nullptr;
		uint32_t Width = 0;
		uint32_t Height = 0;
	};

	class RenderPass {
	public:
		static Ref<RenderPass> Create(const RenderPassCreateInfo& createInfo);

		virtual void Begin(RenderPassBeginInfo& info) = 0;
		virtual void End() = 0;
		virtual void Recreate() = 0;
		virtual void Destroy() = 0;

		inline bool IsDepthBuffered() { return m_DepthBuffered; }

		RenderPass(const RenderPassCreateInfo& createInfo);
		virtual ~RenderPass() = default;

		inline ClearColor GetClearColor() { return m_CreateInfo.ClearColor; }
	protected:
		RenderPassCreateInfo m_CreateInfo;

		bool m_DepthBuffered = false;
	};
}