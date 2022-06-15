#include "lypch.h"
#include "VulkanRenderPass.h"

#include "vulkan/vulkan.h"

#include "Renderer.h"
#include "Buffer/Vulkan/VulkanFrameBuffer.h"
#include "Context/VulkanDevice.h"
#include "Context/VulkanSwapChain.h"

namespace Lucy {

	VulkanRenderPass::VulkanRenderPass(const RenderPassSpecification& specs)
		: RenderPass(specs) {
		RefLucy<VulkanRHIRenderPassDesc> renderPassDesc = As(m_Specs.InternalInfo, VulkanRHIRenderPassDesc);
		//we need this in framebuffer, outside the enqueue function, since otherwise there will be a conflict
		m_DepthBuffered = renderPassDesc->DepthEnable;

		Renderer::Enqueue([this]() {
			Create();
		});
	}

	void VulkanRenderPass::Create() {
		const VulkanDevice& device = VulkanDevice::Get();

		RefLucy<VulkanRHIRenderPassDesc> renderPassDesc = As(m_Specs.InternalInfo, VulkanRHIRenderPassDesc);

		VkAttachmentDescription colorAttachmentDescription{};
		colorAttachmentDescription.format = renderPassDesc->ColorDescriptor.Format;
		colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT; //MSAA but we dont use it yet
		colorAttachmentDescription.loadOp = renderPassDesc->ColorDescriptor.LoadOp;
		colorAttachmentDescription.storeOp = renderPassDesc->ColorDescriptor.StoreOp;
		colorAttachmentDescription.stencilLoadOp = renderPassDesc->ColorDescriptor.StencilLoadOp;
		colorAttachmentDescription.stencilStoreOp = renderPassDesc->ColorDescriptor.StencilStoreOp;
		colorAttachmentDescription.initialLayout = renderPassDesc->ColorDescriptor.InitialLayout;
		colorAttachmentDescription.finalLayout = renderPassDesc->ColorDescriptor.FinalLayout;

		VkAttachmentDescription depthAttachmentDescription{};
		depthAttachmentDescription.format = VK_FORMAT_D32_SFLOAT;
		depthAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentReference{};
		depthAttachmentReference.attachment = 1;
		depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDependency subpassColorDependency;
		subpassColorDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassColorDependency.dstSubpass = 0;
		subpassColorDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassColorDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassColorDependency.srcAccessMask = 0;
		subpassColorDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		subpassColorDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkSubpassDependency subpassDepthDependency;
		subpassDepthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDepthDependency.dstSubpass = 0;
		subpassDepthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		subpassDepthDependency.srcAccessMask = 0;
		subpassDepthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		subpassDepthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		subpassDepthDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkSubpassDescription subpassDescription{};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = renderPassDesc->ColorAttachments.size();
		subpassDescription.pColorAttachments = renderPassDesc->ColorAttachments.data();
		subpassDescription.pDepthStencilAttachment = nullptr;

		std::vector<VkAttachmentDescription> attachments = { colorAttachmentDescription };
		std::vector<VkSubpassDependency> dependencies = { subpassColorDependency };

		if (m_DepthBuffered) {
			attachments.push_back(depthAttachmentDescription);
			dependencies.push_back(subpassDepthDependency);

			subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;
		}

		VkRenderPassCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		createInfo.attachmentCount = attachments.size();
		createInfo.pAttachments = attachments.data();
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subpassDescription;
		createInfo.dependencyCount = dependencies.size();
		createInfo.pDependencies = dependencies.data();

		m_AttachmentCount = createInfo.attachmentCount;

		LUCY_VK_ASSERT(vkCreateRenderPass(device.GetLogicalDevice(), &createInfo, nullptr, &m_RenderPass));
	}

	void VulkanRenderPass::Begin(RenderPassBeginInfo& info) {
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();

		VkRenderPassBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		beginInfo.renderPass = m_RenderPass;
		beginInfo.framebuffer = info.VulkanFrameBuffer;
		beginInfo.renderArea.offset = { 0, 0 };
		beginInfo.renderArea.extent = { info.Width, info.Height };

		VkClearValue clearColor;
		clearColor.color = { m_Specs.ClearColor.r, m_Specs.ClearColor.g, m_Specs.ClearColor.b, m_Specs.ClearColor.a };
		
		VkClearValue clearDepth;
		clearDepth.depthStencil.depth = 1.0f;

		VkClearValue clearValues[2] = { clearColor, clearDepth };
		beginInfo.clearValueCount = 2;
		beginInfo.pClearValues = clearValues;

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
		const VulkanDevice& device = VulkanDevice::Get();
		vkDestroyRenderPass(device.GetLogicalDevice(), m_RenderPass, nullptr);
	}
}