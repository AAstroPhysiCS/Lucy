#include "lypch.h"
#include "VulkanFrameBuffer.h"

#include "vulkan/vulkan.h"

#include "Renderer/Renderer.h"
#include "../../Context/VulkanDevice.h"

namespace Lucy {

	VulkanFrameBuffer::VulkanFrameBuffer(FrameBufferSpecification& specs)
		: FrameBuffer(specs) {
		Renderer::Enqueue([=]() {
			Create();
		});
	}

	void VulkanFrameBuffer::Create() {
		auto& swapChainInstance = VulkanSwapChain::Get();
		m_FrameBufferHandles.resize(swapChainInstance.GetImageCount(), VK_NULL_HANDLE);

		VkDevice device = VulkanDevice::Get().GetLogicalDevice();

		RefLucy<VulkanRHIFrameBufferDesc> frameBufferDesc = As(m_Specs.InternalInfo, VulkanRHIFrameBufferDesc);
		//this would make problems when we resized the window, since the lifetime of this object is short
		if (frameBufferDesc) {
			m_RenderPass = As(frameBufferDesc->RenderPass, VulkanRenderPass);
			m_Images = frameBufferDesc->ImageBuffers;
			m_ImageViews = frameBufferDesc->ImageViews;
		}

		for (uint32_t i = 0; i < swapChainInstance.GetImageCount(); i++) {
			VkImageView imageViewHandle = VK_NULL_HANDLE;
			if (m_ImageViews.empty()) {
				imageViewHandle = m_Images[i]->GetImageView().GetVulkanHandle();
			} else {
				imageViewHandle = m_ImageViews[i].GetVulkanHandle();
			}

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

	void VulkanFrameBuffer::Destroy() {
		VkDevice device = VulkanDevice::Get().GetLogicalDevice();
		for (uint32_t i = 0; i < m_FrameBufferHandles.size(); i++) {
			vkDestroyFramebuffer(device, m_FrameBufferHandles[i], nullptr);
			if (!m_Images.empty())
				m_Images[i]->Destroy();
			//no ownership for imageview is taken, the owner is the swapchain
			//so it should also get destroyed in the swapchain
		}
		m_Specs.InternalInfo = nullptr;
	}

	void VulkanFrameBuffer::Recreate(uint32_t width, uint32_t height, RefLucy<void> internalInfo) {
		m_Specs.Width = width;
		m_Specs.Height = height;
		
		Destroy();
		if (internalInfo)
			m_Specs.InternalInfo = internalInfo;

		for (uint32_t i = 0; i < m_Images.size(); i++) {
			m_Images[i]->Recreate(width, height);
		}
		Create();
	}
}