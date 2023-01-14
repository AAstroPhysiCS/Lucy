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

			VkAttachmentDescription colorAttachment = VulkanAPI::AttachmentDescription((VkFormat)GetAPIImageFormat(attachment.Format), (VkAttachmentLoadOp)GetAPILoadOp(attachment.LoadOp),
																					   (VkAttachmentStoreOp)GetAPIStoreOp(attachment.StoreOp), (VkAttachmentLoadOp)GetAPILoadOp(attachment.StencilLoadOp),
																					   (VkAttachmentStoreOp)GetAPIStoreOp(attachment.StencilStoreOp), (VkSampleCountFlagBits)attachment.Samples,
																					   (VkImageLayout)attachment.Initial, (VkImageLayout)attachment.Final);
			VkAttachmentReference colorAttachmentReference = VulkanAPI::AttachmentReference(i, (VkImageLayout)attachment.Reference.Layout);

			attachmentDescription[i] = colorAttachment;
			attachmentReferences[i] = colorAttachmentReference;
		}

		if (m_DepthBuffered) {
			VkAttachmentDescription depthAttachmentDescription = VulkanAPI::AttachmentDescription((VkFormat)GetAPIImageFormat(depthAttachment.Format), (VkAttachmentLoadOp)GetAPILoadOp(depthAttachment.LoadOp),
																								  (VkAttachmentStoreOp)GetAPIStoreOp(depthAttachment.StoreOp), (VkAttachmentLoadOp)GetAPILoadOp(depthAttachment.StencilLoadOp),
																								  (VkAttachmentStoreOp)GetAPIStoreOp(depthAttachment.StencilStoreOp), (VkSampleCountFlagBits)depthAttachment.Samples,
																								  (VkImageLayout)depthAttachment.Initial, (VkImageLayout)depthAttachment.Final);

			VkAttachmentReference depthAttachmentReference = VulkanAPI::AttachmentReference((uint32_t)colorAttachments.size(), //last element will always be the depth attachment
																							(VkImageLayout)depthAttachment.Reference.Layout);
			attachmentDescription.push_back(depthAttachmentDescription);
			attachmentReferences.push_back(depthAttachmentReference);
		}

		VkSubpassDependency subpassColorDependency = VulkanAPI::SubpassDependency(VK_SUBPASS_EXTERNAL, 0,
																				  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
																				  0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0);
		VkSubpassDependency subpassDepthDependency = VulkanAPI::SubpassDependency(VK_SUBPASS_EXTERNAL, 0,
																				  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
																				  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
																				  0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_DEPENDENCY_BY_REGION_BIT);

		std::vector<VkSubpassDependency> subpassDependencies = { subpassColorDependency };

		VkSubpassDescription subpassDescription = VulkanAPI::SubpassDescription(VK_PIPELINE_BIND_POINT_GRAPHICS, (uint32_t)colorAttachments.size(), attachmentReferences.data(), nullptr);

		if (m_DepthBuffered) {
			subpassDependencies.push_back(subpassDepthDependency);
			subpassDescription.pDepthStencilAttachment = &attachmentReferences[attachmentReferences.size() - 1]; //depth will always be the last one
		}

		VkRenderPassCreateInfo createInfo = VulkanAPI::RenderPassCreateInfo((uint32_t)attachmentDescription.size(), attachmentDescription.data(), 1, &subpassDescription,
																			(uint32_t)subpassDependencies.size(), subpassDependencies.data());
		m_AttachmentCount = createInfo.attachmentCount;
		m_ColorAttachmentCount = (uint32_t)colorAttachments.size();

		VkRenderPassMultiviewCreateInfo renderPassMultiview = VulkanAPI::RenderPassMultiviewCreateInfo(createInfo.subpassCount, &m_CreateInfo.Multiview.ViewMask, 1, &m_CreateInfo.Multiview.CorrelationMask);
		if (m_CreateInfo.Multiview.IsValid())
			createInfo.pNext = &renderPassMultiview;

		LUCY_VK_ASSERT(vkCreateRenderPass(device.GetLogicalDevice(), &createInfo, nullptr, &m_RenderPass));
	}

	void VulkanRenderPass::Begin(VulkanRenderPassBeginInfo& info) {
		const VulkanSwapChain& swapChain = VulkanSwapChain::Get();

		VkClearValue clearColor;
		clearColor.color = { m_CreateInfo.ClearColor.r, m_CreateInfo.ClearColor.g, m_CreateInfo.ClearColor.b, m_CreateInfo.ClearColor.a };

		VkClearValue clearDepth;
		clearDepth.depthStencil.depth = 1.0f;

		std::vector<VkClearValue> clearValues(m_ColorAttachmentCount, clearColor);
		if (m_DepthBuffered)
			clearValues.push_back(clearDepth);

		m_BoundedCommandBuffer = info.CommandBuffer;

		VkRenderPassBeginInfo beginInfo = VulkanAPI::RenderPassBeginInfo(m_RenderPass, info.VulkanFrameBuffer, { { 0, 0 }, { info.Width, info.Height } }, (uint32_t)clearValues.size(), clearValues.data());
		vkCmdBeginRenderPass(m_BoundedCommandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport;
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = (float)info.Width;
		viewport.height = (float)info.Height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		LUCY_ASSERT(viewport.width != 0 || viewport.height != 0);

		VkRect2D scissor;
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
		m_RenderPass = VK_NULL_HANDLE;
	}
}