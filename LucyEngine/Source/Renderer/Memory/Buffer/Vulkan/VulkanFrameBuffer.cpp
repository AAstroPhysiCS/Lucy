#include "lypch.h"
#include "VulkanFrameBuffer.h"

#include "vulkan/vulkan.h"

#include "Renderer/Renderer.h"
#include "Renderer/Context/VulkanContextDevice.h"

namespace Lucy {

	VulkanFrameBuffer::VulkanFrameBuffer(FrameBufferCreateInfo& createInfo)
		: FrameBuffer(createInfo) {
		m_RenderPass = m_CreateInfo.RenderPass.As<VulkanRenderPass>();
		m_Images = std::vector<Ref<VulkanImage2D>>(m_CreateInfo.ImageBuffers.begin(), m_CreateInfo.ImageBuffers.end());

		if (m_CreateInfo.DepthImage)
			m_DepthImage = m_CreateInfo.DepthImage.As<VulkanImage2D>();

		if (!m_CreateInfo.ImageViews.empty()) {
			Renderer::EnqueueToRenderThread([=]() {
				CreateForSwapChain();
			});
			return;
		}

		if (m_CreateInfo.ImageBuffers.empty()) {
			LUCY_CRITICAL("Neither a imageview nor a imagebuffer is defined!");
			LUCY_ASSERT(false);
		}

		Renderer::EnqueueToRenderThread([=]() {
			Create();
		});
	}

	void VulkanFrameBuffer::Create() {
		if (m_CreateInfo.IsInFlight)
			m_FrameBufferHandles.resize(Renderer::GetMaxFramesInFlight(), VK_NULL_HANDLE);
		else
			m_FrameBufferHandles.resize(1, VK_NULL_HANDLE);

		VkDevice device = VulkanContextDevice::Get().GetLogicalDevice();

		//because we dont want to recreate additional in flight frames everytime when we resize
		if (!m_CreatedInFlightFrameBufferImages && m_CreateInfo.IsInFlight) {
			//imageCount is here, so that we dont loop over and over endlessly
			size_t imageCount = m_Images.size();
			for (uint32_t i = 0; i < imageCount; i++) {
				for (uint32_t j = 0; j < m_FrameBufferHandles.size() - 1; j++) {
					m_Images.push_back(Image::Create(m_Images[i]));
				}
			}
			m_CreatedInFlightFrameBufferImages = true;
		}

		for (uint32_t i = 0; i < m_FrameBufferHandles.size(); i++) {
			std::vector<VkImageView> imageViewHandles;
			imageViewHandles.reserve(m_RenderPass->GetColorAttachmentCount());

			if (m_CreateInfo.IsInFlight) {
				for (uint32_t j = 0; j < imageViewHandles.capacity(); j++)
					imageViewHandles.push_back(m_Images[i + j]->GetImageView().GetVulkanHandle());
			} else {
				for (uint32_t j = 0; j < m_Images.size(); j++)
					imageViewHandles.push_back(m_Images[j]->GetImageView().GetVulkanHandle());
			}

			if (m_RenderPass->IsDepthBuffered())
				imageViewHandles.push_back(m_DepthImage->GetImageView().GetVulkanHandle());

			VkFramebufferCreateInfo createInfo = VulkanAPI::FramebufferCreateInfo(m_RenderPass->GetVulkanHandle(), (uint32_t)imageViewHandles.size(), imageViewHandles.data(), m_CreateInfo.Width, m_CreateInfo.Height, 1);
			LUCY_VK_ASSERT(vkCreateFramebuffer(device, &createInfo, nullptr, &m_FrameBufferHandles[i]));
		}
	}

	void VulkanFrameBuffer::CreateForSwapChain() {
		m_FrameBufferHandles.resize(Renderer::GetMaxFramesInFlight(), VK_NULL_HANDLE);

		VkDevice device = VulkanContextDevice::Get().GetLogicalDevice();

		for (uint32_t i = 0; i < m_FrameBufferHandles.size(); i++) {
			VkImageView swapChainView = m_CreateInfo.ImageViews[i].GetVulkanHandle();

			VkFramebufferCreateInfo createInfo = VulkanAPI::FramebufferCreateInfo(m_RenderPass->GetVulkanHandle(), 1, &swapChainView, m_CreateInfo.Width, m_CreateInfo.Height, 1);
			LUCY_VK_ASSERT(vkCreateFramebuffer(device, &createInfo, nullptr, &m_FrameBufferHandles[i]));
		}
	}

	void VulkanFrameBuffer::Destroy() {
		VkDevice device = VulkanContextDevice::Get().GetLogicalDevice();

		//no ownership for imageview is taken, the owner is the swapchain
		//so it should also get destroyed in the swapchain
		//we just destroy the image buffers and the depth
		for (Ref<Image> image : m_Images)
			image->Destroy();

		if (m_DepthImage)
			m_DepthImage->Destroy();

		for (uint32_t i = 0; i < m_FrameBufferHandles.size(); i++) {
			if (m_FrameBufferHandles[i])
				vkDestroyFramebuffer(device, m_FrameBufferHandles[i], nullptr);
			m_FrameBufferHandles[i] = VK_NULL_HANDLE;
		}
	}

	void VulkanFrameBuffer::Recreate(uint32_t width, uint32_t height) {
		Recreate(width, height, {});
	}

	void VulkanFrameBuffer::Recreate(uint32_t width, uint32_t height, const std::vector<VulkanImageView>& swapChainImageViews) {
		m_CreateInfo.Width = width;
		m_CreateInfo.Height = height;
		m_CreateInfo.ImageViews = swapChainImageViews;

		Destroy();

		//replacing the newly created image views from the swapchain
		//disclaimer: it is only for swapchain purposes only
		if (m_CreateInfo.ImageViews.size() != 0) {
			//normally, recreating renderpass here shouldn't be allowed. But swapchain is an exception
			m_RenderPass->Recreate();
		}

		for (uint32_t i = 0; i < m_Images.size(); i++)
			m_Images[i]->Recreate(width, height);

		if (m_DepthImage)
			m_DepthImage->Recreate(width, height);

		if (!m_CreateInfo.ImageViews.empty()) {
			CreateForSwapChain();
			return;
		}
		Create();
	}
}