#include "lypch.h"
#include "VulkanFrameBuffer.h"

#include "vulkan/vulkan.h"

#include "Renderer/Renderer.h"
#include "../../Context/VulkanDevice.h"

namespace Lucy {

	VulkanFrameBuffer::VulkanFrameBuffer(FrameBufferSpecification& specs)
		: FrameBuffer(specs) {
		RefLucy<VulkanRHIFrameBufferDesc> frameBufferDesc = As(m_Specs.InternalInfo, VulkanRHIFrameBufferDesc);
		//this would make problems when we resized the window, since the lifetime of this object is short
		if (frameBufferDesc) {
			m_RenderPass = As(frameBufferDesc->RenderPass, VulkanRenderPass);
			m_Images = frameBufferDesc->ImageBuffers;
			m_ImageViews = frameBufferDesc->ImageViews;
		}

		if (m_RenderPass->IsDepthBuffered())
			CreateDepthImage();

		Renderer::Enqueue([=]() {
			Create();
		});
	}

	void VulkanFrameBuffer::Create() {
		auto& swapChainInstance = VulkanSwapChain::Get();
		m_FrameBufferHandles.resize(swapChainInstance.GetImageCount(), VK_NULL_HANDLE);

		VkDevice device = VulkanDevice::Get().GetLogicalDevice();

		for (uint32_t i = 0; i < swapChainInstance.GetImageCount(); i++) {
			VkImageView imageViewHandle[2] = { VK_NULL_HANDLE, VK_NULL_HANDLE };
			if (m_ImageViews.empty())
				imageViewHandle[0] = m_Images[i]->GetImageView().GetVulkanHandle();
			else
				imageViewHandle[0] = m_ImageViews[i].GetVulkanHandle();

			//we dont need to create 3 depth buffers, we can create 1 and reuse it.
			if (m_RenderPass->IsDepthBuffered())
				imageViewHandle[1] = m_DepthImage->GetImageView().GetVulkanHandle();

			VkFramebufferCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			createInfo.renderPass = m_RenderPass->GetVulkanHandle();
			createInfo.attachmentCount = m_RenderPass->GetAttachmentCount();
			createInfo.pAttachments = imageViewHandle;
			createInfo.width = m_Specs.Width;
			createInfo.height = m_Specs.Height;
			createInfo.layers = 1;

			LUCY_VK_ASSERT(vkCreateFramebuffer(device, &createInfo, nullptr, &m_FrameBufferHandles[i]));
		}
	}

	void VulkanFrameBuffer::CreateDepthImage() {
		ImageSpecification depthImageSpecs;
		depthImageSpecs.ImageType = ImageType::Type2D;
		depthImageSpecs.Format = VK_FORMAT_D32_SFLOAT;
		depthImageSpecs.Width = m_Specs.Width;
		depthImageSpecs.Height = m_Specs.Height;
		auto& desc = CreateRef<VulkanRHIImageDesc>();
		desc->DepthEnable = true;
		depthImageSpecs.InternalInfo = desc;

		m_DepthImage = As(Image2D::Create(depthImageSpecs), VulkanImage2D);
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

		if (m_DepthImage)
			m_DepthImage->Destroy();

		m_Specs.InternalInfo = nullptr;
	}

	void VulkanFrameBuffer::Recreate(uint32_t width, uint32_t height, RefLucy<void> internalInfo) {
		m_Specs.Width = width;
		m_Specs.Height = height;

		Destroy();
		if (internalInfo) {
			m_Specs.InternalInfo = internalInfo;

			RefLucy<VulkanRHIFrameBufferDesc> frameBufferDesc = As(m_Specs.InternalInfo, VulkanRHIFrameBufferDesc);
			m_RenderPass = As(frameBufferDesc->RenderPass, VulkanRenderPass);
			m_Images = frameBufferDesc->ImageBuffers;
			m_ImageViews = frameBufferDesc->ImageViews;
		}

		for (uint32_t i = 0; i < m_Images.size(); i++)
			m_Images[i]->Recreate(width, height);

		if (m_DepthImage)
			m_DepthImage->Recreate(width, height);

		Create();
	}
}