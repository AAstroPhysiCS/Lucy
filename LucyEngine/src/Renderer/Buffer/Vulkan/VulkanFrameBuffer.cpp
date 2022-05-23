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
		//if its the first initialization (the object is null
		//this would make problems when we resized the window, since the lifetime of that object is short
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

	void VulkanFrameBuffer::Bind() {

	}

	void VulkanFrameBuffer::Unbind() {

	}

	void VulkanFrameBuffer::Destroy() {
		VkDevice device = VulkanDevice::Get().GetLogicalDevice();
		for (uint32_t i = 0; i < m_FrameBufferHandles.size(); i++) {
			vkDestroyFramebuffer(device, m_FrameBufferHandles[i], nullptr);
			if (!m_Images.empty())
				m_Images[i]->Destroy();
		}
		m_Specs.InternalInfo = nullptr;
	}

	void VulkanFrameBuffer::Recreate() {
		Destroy();
		for (uint32_t i = 0; i < m_Images.size(); i++) {
			m_Images[i]->Recreate();
		}
		Create();
	}
}