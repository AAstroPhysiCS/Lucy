#include "lypch.h"
#include "VulkanFrameBuffer.h"

#include "vulkan/vulkan.h"

#include "Renderer/Renderer.h"
#include "../../Context/VulkanDevice.h"

namespace Lucy {

	VulkanFrameBuffer::VulkanFrameBuffer(FrameBufferSpecification& specs, RefLucy<VulkanRenderPass>& renderPass)
		: FrameBuffer(specs), m_RenderPass(renderPass) {
		Renderer::Submit([this]() {
			Create();
		});
	}

	void VulkanFrameBuffer::Create() {
		auto& swapChainInstance = VulkanSwapChain::Get();
		m_FrameBufferHandles.resize(swapChainInstance.GetImageCount());

		VkDevice device = VulkanDevice::Get().GetLogicalDevice();

		for (uint32_t i = 0; i < swapChainInstance.GetImageCount(); i++) {
			VkImageView imageViewHandle = m_Specs.ImageViews[i].GetVulkanHandle();

			VkFramebufferCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			createInfo.renderPass = m_RenderPass->GetVulkanHandle();
			createInfo.attachmentCount = 1;
			createInfo.pAttachments = &imageViewHandle;
			createInfo.width = m_Specs.Width;
			createInfo.height = m_Specs.Height;
			createInfo.layers = 1;

			LUCY_VK_ASSERT(vkCreateFramebuffer(device, &createInfo, nullptr, &m_FrameBufferHandles[i]));
		}
	}

	void VulkanFrameBuffer::Bind() {

	}

	void VulkanFrameBuffer::Unbind() {

	}

	void VulkanFrameBuffer::Destroy() {
		VkDevice device = VulkanDevice::Get().GetLogicalDevice();
		for (uint32_t i = 0; i < m_FrameBufferHandles.size(); i++) {
			vkDestroyFramebuffer(device, m_FrameBufferHandles[i], nullptr);
		}
	}

	void VulkanFrameBuffer::Recreate() {
		Destroy();
		Create();
	}
}