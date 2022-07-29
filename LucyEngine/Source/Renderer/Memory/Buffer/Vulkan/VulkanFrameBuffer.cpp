#include "lypch.h"
#include "VulkanFrameBuffer.h"

#include "vulkan/vulkan.h"

#include "Renderer/Renderer.h"
#include "Renderer/Context/VulkanDevice.h"
#include "Renderer/Context/VulkanSwapChain.h"

namespace Lucy {

	VulkanFrameBuffer::VulkanFrameBuffer(FrameBufferCreateInfo& createInfo)
		: FrameBuffer(createInfo) {
		Ref<VulkanFrameBufferInfo> frameBufferInfo = m_CreateInfo.InternalInfo.As<VulkanFrameBufferInfo>();
		//this would make problems when we resized the window, since the lifetime of this object is short
		if (frameBufferInfo) {
			m_RenderPass = frameBufferInfo->RenderPass.As<VulkanRenderPass>();
			m_ImageViews = frameBufferInfo->ImageViews;

			m_Images = std::vector<Ref<VulkanImage2D>>(m_CreateInfo.ImageBuffers.begin(), m_CreateInfo.ImageBuffers.end());
		}

		if (m_RenderPass->IsDepthBuffered())
			CreateDepthImage();

		Renderer::EnqueueToRenderThread([=]() {
			Create();
		});
	}

	void VulkanFrameBuffer::Create() {
		auto& swapChainInstance = VulkanSwapChain::Get();
		if (!m_ImageViews.empty()) { //if the framebuffer is being used for swapchain
			m_FrameBufferHandles.resize(swapChainInstance.GetImageCount(), VK_NULL_HANDLE);
		} else {
			m_FrameBufferHandles.resize(swapChainInstance.GetMaxFramesInFlight(), VK_NULL_HANDLE);
		}

		VkDevice device = VulkanDevice::Get().GetLogicalDevice();

		for (uint32_t i = 0; i < m_FrameBufferHandles.size(); i++) {
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
			createInfo.width = m_CreateInfo.Width;
			createInfo.height = m_CreateInfo.Height;
			createInfo.layers = 1;

			LUCY_VK_ASSERT(vkCreateFramebuffer(device, &createInfo, nullptr, &m_FrameBufferHandles[i]));
		}
	}

	void VulkanFrameBuffer::CreateDepthImage() {
		ImageCreateInfo depthImageCreateInfo;
		depthImageCreateInfo.ImageType = ImageType::Type2D;
		depthImageCreateInfo.Target = ImageTarget::Depth;
		depthImageCreateInfo.Format = VK_FORMAT_D32_SFLOAT;
		depthImageCreateInfo.Width = m_CreateInfo.Width;
		depthImageCreateInfo.Height = m_CreateInfo.Height;

		m_DepthImage = Image2D::Create(depthImageCreateInfo).As<VulkanImage2D>();
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

		m_CreateInfo.InternalInfo = nullptr;
	}

	void VulkanFrameBuffer::Recreate(uint32_t width, uint32_t height, Ref<void> internalInfo) {
		m_CreateInfo.Width = width;
		m_CreateInfo.Height = height;

		Destroy();
		if (internalInfo) {
			m_CreateInfo.InternalInfo = internalInfo;

			Ref<VulkanFrameBufferInfo> frameBufferInfo = m_CreateInfo.InternalInfo.As<VulkanFrameBufferInfo>();
			m_RenderPass = frameBufferInfo->RenderPass.As<VulkanRenderPass>();
			m_ImageViews = frameBufferInfo->ImageViews;

			m_Images = std::vector<Ref<VulkanImage2D>>(m_CreateInfo.ImageBuffers.begin(), m_CreateInfo.ImageBuffers.end());
		}

		for (uint32_t i = 0; i < m_Images.size(); i++)
			m_Images[i]->Recreate(width, height);

		if (m_DepthImage)
			m_DepthImage->Recreate(width, height);
		Create();
	}
}