#include "lypch.h"
#include "VulkanRenderPass.h"

#include "Renderer/Renderer.h"
#include "Context/VulkanContextDevice.h"
#include "Context/VulkanSwapChain.h"

namespace Lucy {

	VulkanRenderPass::VulkanRenderPass(const RenderPassCreateInfo& createInfo)
		: RenderPass(createInfo) {
		m_DepthBuffered = m_CreateInfo.Layout.DepthAttachment.IsValid();

		Renderer::EnqueueToRenderThread([this]() {
			Create();
		});
	}

	void VulkanRenderPass::Create() {
		const VulkanContextDevice& device = VulkanContextDevice::Get();

		const std::vector<RenderPassLayout::Attachment>& colorAttachments = m_CreateInfo.Layout.ColorAttachments;
		const RenderPassLayout::Attachment& depthAttachment = m_CreateInfo.Layout.DepthAttachment;

		std::vector<VkAttachmentDescription> attachmentDescription;
		std::vector<VkAttachmentReference> attachmentReferences;
		attachmentDescription.resize(colorAttachments.size());
		attachmentReferences.resize(colorAttachments.size());

		for (uint32_t i = 0; i < colorAttachments.size(); i++) {
			const RenderPassLayout::Attachment& attachment = colorAttachments[i];

			VkAttachmentDescription colorAttachment{};
			colorAttachment.format = (VkFormat)GetAPIImageFormat(attachment.Format);
			colorAttachment.loadOp = (VkAttachmentLoadOp)GetAPILoadOp(attachment.LoadOp);
			colorAttachment.storeOp = (VkAttachmentStoreOp)GetAPIStoreOp(attachment.StoreOp);
			colorAttachment.stencilLoadOp = (VkAttachmentLoadOp)GetAPILoadOp(attachment.StencilLoadOp);
			colorAttachment.stencilStoreOp = (VkAttachmentStoreOp)GetAPIStoreOp(attachment.StencilStoreOp);
			colorAttachment.samples = (VkSampleCountFlagBits)attachment.Samples;
			colorAttachment.initialLayout = (VkImageLayout)attachment.Initial;
			colorAttachment.finalLayout = (VkImageLayout)attachment.Final;

			VkAttachmentReference colorAttachmentReference{};
			colorAttachmentReference.attachment = i;
			colorAttachmentReference.layout = (VkImageLayout)attachment.Reference.Layout;

			attachmentDescription[i] = colorAttachment;
			attachmentReferences[i] = colorAttachmentReference;
		}

		if (m_DepthBuffered) {
			VkAttachmentDescription depthAttachmentDescription{};
			depthAttachmentDescription.format = (VkFormat)GetAPIImageFormat(depthAttachment.Format);
			depthAttachmentDescription.loadOp = (VkAttachmentLoadOp)GetAPILoadOp(depthAttachment.LoadOp);
			depthAttachmentDescription.storeOp = (VkAttachmentStoreOp)GetAPIStoreOp(depthAttachment.StoreOp);
			depthAttachmentDescription.stencilLoadOp = (VkAttachmentLoadOp)GetAPILoadOp(depthAttachment.StencilLoadOp);
			depthAttachmentDescription.stencilStoreOp = (VkAttachmentStoreOp)GetAPIStoreOp(depthAttachment.StencilStoreOp);
			depthAttachmentDescription.samples = (VkSampleCountFlagBits)depthAttachment.Samples;
			depthAttachmentDescription.initialLayout = (VkImageLayout)depthAttachment.Initial;
			depthAttachmentDescription.finalLayout = (VkImageLayout)depthAttachment.Final;

			VkAttachmentReference depthAttachmentReference{};
			depthAttachmentReference.attachment = colorAttachments.size(); //last element will always be the depth attachment
			depthAttachmentReference.layout = (VkImageLayout)depthAttachment.Reference.Layout;

			attachmentDescription.push_back(depthAttachmentDescription);
			attachmentReferences.push_back(depthAttachmentReference);
		}

		VkSubpassDependency subpassColorDependency{};
		subpassColorDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassColorDependency.dstSubpass = 0;
		subpassColorDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassColorDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassColorDependency.srcAccessMask = 0;
		subpassColorDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassColorDependency.dependencyFlags = 0;

		VkSubpassDependency subpassDepthDependency{};
		subpassDepthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDepthDependency.dstSubpass = 0;
		subpassDepthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		subpassDepthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		subpassDepthDependency.srcAccessMask = 0;
		subpassDepthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		subpassDepthDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		std::vector<VkSubpassDependency> subpassDependencies = { subpassColorDependency };

		VkSubpassDescription subpassDescription{};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = colorAttachments.size();
		subpassDescription.pColorAttachments = attachmentReferences.data();
		subpassDescription.pDepthStencilAttachment = nullptr;
		
		if (m_DepthBuffered) {
			subpassDependencies.push_back(subpassDepthDependency);

			subpassDescription.pDepthStencilAttachment = &attachmentReferences[attachmentReferences.size() - 1]; //depth will always be the last one
		}

		VkRenderPassCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		createInfo.attachmentCount = attachmentDescription.size();
		createInfo.pAttachments = attachmentDescription.data();
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subpassDescription;
		createInfo.dependencyCount = subpassDependencies.size();
		createInfo.pDependencies = subpassDependencies.data();

		m_AttachmentCount = createInfo.attachmentCount;
		m_ColorAttachmentCount = colorAttachments.size();

		VkRenderPassMultiviewCreateInfo renderPassMultiview{};
		if (m_CreateInfo.Multiview.IsValid()) {
			renderPassMultiview.sType = VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO;
			renderPassMultiview.subpassCount = createInfo.subpassCount;
			renderPassMultiview.pViewMasks = &m_CreateInfo.Multiview.ViewMask;
			renderPassMultiview.correlationMaskCount = 1;
			renderPassMultiview.pCorrelationMasks = &m_CreateInfo.Multiview.CorrelationMask;

			createInfo.pNext = &renderPassMultiview;
		}

		LUCY_VK_ASSERT(vkCreateRenderPass(device.GetLogicalDevice(), &createInfo, nullptr, &m_RenderPass));
	}

	void VulkanRenderPass::Begin(VulkanRenderPassBeginInfo& info) {
		const VulkanSwapChain& swapChain = VulkanSwapChain::Get();

		VkRenderPassBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		beginInfo.renderPass = m_RenderPass;
		beginInfo.framebuffer = info.VulkanFrameBuffer;
		beginInfo.renderArea.offset = { 0, 0 };
		beginInfo.renderArea.extent = { info.Width, info.Height };

		VkClearValue clearColor;
		clearColor.color = { m_CreateInfo.ClearColor.r, m_CreateInfo.ClearColor.g, m_CreateInfo.ClearColor.b, m_CreateInfo.ClearColor.a };

		VkClearValue clearDepth;
		clearDepth.depthStencil.depth = 1.0f;

		std::vector<VkClearValue> clearValues(m_ColorAttachmentCount, clearColor);
		if (m_DepthBuffered)
			clearValues.push_back(clearDepth);

		beginInfo.clearValueCount = clearValues.size();
		beginInfo.pClearValues = clearValues.data();

		m_BoundedCommandBuffer = info.CommandBuffer;
		vkCmdBeginRenderPass(m_BoundedCommandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport;
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = info.Width;
		viewport.height = info.Height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		if (viewport.width == 0 || viewport.height == 0)
			LUCY_ASSERT(false);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = { info.Width, info.Height };

		vkCmdSetViewport(m_BoundedCommandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(m_BoundedCommandBuffer, 0, 1, &scissor);
	}

	void VulkanRenderPass::End() {
		vkCmdEndRenderPass(m_BoundedCommandBuffer);
	}

	void VulkanRenderPass::Recreate() {
		Destroy();
		Create();
	}

	void VulkanRenderPass::Destroy() {
		const VulkanContextDevice& device = VulkanContextDevice::Get();
		vkDestroyRenderPass(device.GetLogicalDevice(), m_RenderPass, nullptr);
	}
}