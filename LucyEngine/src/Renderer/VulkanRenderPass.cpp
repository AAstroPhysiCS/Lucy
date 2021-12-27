#include "lypch.h"
#include "VulkanRenderPass.h"

#include "vulkan/vulkan.h"

#include "Context/VulkanDevice.h"
#include "Context/VulkanSwapChain.h"

namespace Lucy {

	VulkanRenderPass::VulkanRenderPass(RenderPassSpecification& specs)
		: RenderPass(specs) {
		Create();
	}

	void VulkanRenderPass::Create() {
		VulkanDevice& device = VulkanDevice::Get();
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();

		VkAttachmentDescription colorAttachmentDescription{};
		colorAttachmentDescription.format = swapChain.GetSurfaceFormat().format;
		colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT; //MSAA but we dont use it yet

		colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;

		colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpassDescription{};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &colorAttachmentRef;

		VkSubpassDependency subpassDependency{};
		subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependency.dstSubpass = 0;
		subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.srcAccessMask = 0;
		subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		std::vector<VkAttachmentDescription> attachments = { colorAttachmentDescription };

		VkRenderPassCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		createInfo.attachmentCount = attachments.size();
		createInfo.pAttachments = attachments.data();
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subpassDescription;
		createInfo.dependencyCount = 1;
		createInfo.pDependencies = &subpassDependency;

		LUCY_VULKAN_ASSERT(vkCreateRenderPass(device.GetLogicalDevice(), &createInfo, nullptr, &m_RenderPass));
	}

	void VulkanRenderPass::Begin(RenderPassBeginInfo& info) {
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		
		VkRenderPassBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		beginInfo.renderPass = m_RenderPass;
		beginInfo.framebuffer = info.FrameBuffer;
		beginInfo.renderArea.offset = { 0, 0 };
		beginInfo.renderArea.extent = swapChain.GetExtent();

		VkClearValue clearColor = {{{m_Specs.ClearColor.r, m_Specs.ClearColor.g, m_Specs.ClearColor.b, m_Specs.ClearColor.a}}};
		beginInfo.clearValueCount = 1;
		beginInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(info.CommandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	void VulkanRenderPass::End(RenderPassEndInfo& info) {
		vkCmdEndRenderPass(info.CommandBuffer);
	}

	void VulkanRenderPass::Destroy() {
		VulkanDevice& device = VulkanDevice::Get();
		vkDestroyRenderPass(device.GetLogicalDevice(), m_RenderPass, nullptr);
	}
}