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
		Renderer::Enqueue([this]() {
			Create();
		});
	}

	void VulkanRenderPass::Create() {
		const VulkanDevice& device = VulkanDevice::Get();

		RefLucy<VulkanRHIRenderPassDesc> renderPassDesc = As(m_Specs.InternalInfo, VulkanRHIRenderPassDesc);

		VkAttachmentDescription colorAttachmentDescription{};
		colorAttachmentDescription.format = renderPassDesc->Descriptor.Format;
		colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT; //MSAA but we dont use it yet
		colorAttachmentDescription.loadOp = renderPassDesc->Descriptor.LoadOp;
		colorAttachmentDescription.storeOp = renderPassDesc->Descriptor.StoreOp;
		colorAttachmentDescription.stencilLoadOp = renderPassDesc->Descriptor.StencilLoadOp;
		colorAttachmentDescription.stencilStoreOp = renderPassDesc->Descriptor.StencilStoreOp;
		colorAttachmentDescription.initialLayout = renderPassDesc->Descriptor.InitialLayout;
		colorAttachmentDescription.finalLayout = renderPassDesc->Descriptor.FinalLayout;

		VkSubpassDescription subpassDescription{};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = renderPassDesc->AttachmentReferences.size();
		subpassDescription.pColorAttachments = renderPassDesc->AttachmentReferences.data();

		VkSubpassDependency vkDependency;
		vkDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		vkDependency.dstSubpass = 0;
		vkDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		vkDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		vkDependency.srcAccessMask = 0;
		vkDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		vkDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		std::vector<VkAttachmentDescription> attachments = { colorAttachmentDescription };
		std::vector<VkSubpassDependency> dependencies = { vkDependency };

		VkRenderPassCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		createInfo.attachmentCount = attachments.size();
		createInfo.pAttachments = attachments.data();
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subpassDescription;
		createInfo.dependencyCount = dependencies.size();
		createInfo.pDependencies = dependencies.data();

		LUCY_VK_ASSERT(vkCreateRenderPass(device.GetLogicalDevice(), &createInfo, nullptr, &m_RenderPass));
	}

	void VulkanRenderPass::Begin(RenderPassBeginInfo& info) {
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();

		VkRenderPassBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		beginInfo.renderPass = m_RenderPass;
		beginInfo.framebuffer = info.VulkanFrameBuffer;
		beginInfo.renderArea.offset = { 0, 0 };
		beginInfo.renderArea.extent = swapChain.GetExtent();

		VkClearValue clearColor = { {{m_Specs.ClearColor.r, m_Specs.ClearColor.g, m_Specs.ClearColor.b, m_Specs.ClearColor.a}} };
		beginInfo.clearValueCount = 1;
		beginInfo.pClearValues = &clearColor;

		m_BoundedCommandBuffer = info.CommandBuffer;
		vkCmdBeginRenderPass(m_BoundedCommandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	void VulkanRenderPass::End() {
		vkCmdEndRenderPass(m_BoundedCommandBuffer);
	}

	void VulkanRenderPass::Recreate() {
		Renderer::Enqueue([&]() {
			Destroy();
			Create();
		});
	}

	void VulkanRenderPass::Destroy() {
		const VulkanDevice& device = VulkanDevice::Get();
		vkDestroyRenderPass(device.GetLogicalDevice(), m_RenderPass, nullptr);
	}
}