#include "lypch.h"
#include "VulkanFrameBuffer.h"

#include "vulkan/vulkan.h"

#include "Renderer/Renderer.h"
#include "Renderer/Device/VulkanRenderDevice.h"

namespace Lucy {

	VulkanFrameBuffer::VulkanFrameBuffer(const FrameBufferCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device)
		: FrameBuffer(createInfo), m_ImageHandles(m_CreateInfo.ImageBufferHandles), m_DepthImageHandle(createInfo.DepthImageHandle), m_VulkanDevice(device) {
		LUCY_ASSERT(!m_ImageHandles.empty() || m_DepthImageHandle != InvalidRenderResourceHandle, "Imagebuffer and depth is empty!");
		
		RTCreate();
	}

	void VulkanFrameBuffer::RTCreate() {
		const auto& renderPass = GetRenderPass();

		if (m_CreateInfo.IsInFlight)
			m_FrameBufferHandles.resize(Renderer::GetMaxFramesInFlight(), VK_NULL_HANDLE);
		else
			m_FrameBufferHandles.resize(1, VK_NULL_HANDLE);

		//because we dont want to recreate additional in flight frames everytime when we resize
		if (!m_CreatedInFlightFrameBufferImages && m_CreateInfo.IsInFlight) {
			//imageCount is here, so that we dont loop over and over endlessly
			size_t imageCount = m_ImageHandles.size();
			for (uint32_t i = 0; i < imageCount; i++) {
				for (uint32_t j = 0; j < m_FrameBufferHandles.size() - 1; j++) {
					m_ImageHandles.push_back(m_VulkanDevice->CreateImage(m_VulkanDevice->AccessResource<VulkanImage2D>(m_ImageHandles[i])));
				}
			}
			m_CreatedInFlightFrameBufferImages = true;
		}

		for (uint32_t i = 0; i < m_FrameBufferHandles.size(); i++) {
			std::vector<VkImageView> imageViewHandles;
			imageViewHandles.reserve(renderPass->GetColorAttachmentCount());

			if (m_CreateInfo.IsInFlight) {
				for (uint32_t j = 0; j < imageViewHandles.capacity(); j++)
					imageViewHandles.push_back(GetImage(i + j)->GetImageView().GetVulkanHandle());
			} else {
				for (uint32_t j = 0; j < m_ImageHandles.size(); j++)
					imageViewHandles.push_back(GetImage(j)->GetImageView().GetVulkanHandle());
			}

			if (renderPass->IsDepthBuffered())
				imageViewHandles.push_back(GetDepthImage()->GetImageView().GetVulkanHandle());

			VkFramebufferCreateInfo createInfo = VulkanAPI::FramebufferCreateInfo(renderPass->GetVulkanHandle(), (uint32_t)imageViewHandles.size(), imageViewHandles.data(), m_CreateInfo.Width, m_CreateInfo.Height, 1);
			LUCY_VK_ASSERT(vkCreateFramebuffer(m_VulkanDevice->GetLogicalDevice(), &createInfo, nullptr, &m_FrameBufferHandles[i]));
		}
	}

	Ref<VulkanImage2D> VulkanFrameBuffer::GetImage(uint32_t index) {
		return Renderer::AccessResource<VulkanImage2D>(m_ImageHandles[index]);
	}

	Ref<VulkanImage2D> VulkanFrameBuffer::GetDepthImage() {
		return Renderer::AccessResource<VulkanImage2D>(m_DepthImageHandle);
	}

	Ref<VulkanRenderPass> VulkanFrameBuffer::GetRenderPass() {
		return Renderer::AccessResource<VulkanRenderPass>(m_CreateInfo.RenderPassHandle);
	}

	void VulkanFrameBuffer::RTRecreate(uint32_t width, uint32_t height) {
		m_CreateInfo.Width = width;
		m_CreateInfo.Height = height;

		DestroyHandles();

		for (uint32_t i = 0; i < m_ImageHandles.size(); i++)
			GetImage(i)->RTRecreate(width, height);

		if (Renderer::IsValidRenderResource(m_DepthImageHandle))
			GetDepthImage()->RTRecreate(width, height);

		RTCreate();
	}

	void VulkanFrameBuffer::RTDestroyResource() {
		for (auto imageHandle : m_ImageHandles)
			m_VulkanDevice->RTDestroyResource(imageHandle);

		if (Renderer::IsValidRenderResource(m_DepthImageHandle))
			m_VulkanDevice->RTDestroyResource(m_DepthImageHandle);
		DestroyHandles();
	}

	void VulkanFrameBuffer::DestroyHandles() {
		for (uint32_t i = 0; i < m_FrameBufferHandles.size(); i++) {
			if (m_FrameBufferHandles[i])
				vkDestroyFramebuffer(m_VulkanDevice->GetLogicalDevice(), m_FrameBufferHandles[i], nullptr);
			m_FrameBufferHandles[i] = VK_NULL_HANDLE;
		}
	}

	VulkanSwapChainFrameBuffer::VulkanSwapChainFrameBuffer(const Ref<VulkanRenderDevice>& vulkanDevice, const VkExtent2D& extent, const std::vector<VulkanImageView>& swapChainImageViews, const Ref<VulkanRenderPass>& renderPass)
		: FrameBuffer(FrameBufferCreateInfo{ .Width = extent.width, .Height = extent.height }), m_SwapChainImageViews(swapChainImageViews), m_VulkanDevice(vulkanDevice), m_RenderPass(renderPass) {
		LUCY_ASSERT(!m_SwapChainImageViews.empty(), "Swapchain image views are empty!");
		CreateForSwapChain();
	}

	void VulkanSwapChainFrameBuffer::CreateForSwapChain() {
		m_FrameBufferHandles.resize(m_SwapChainImageViews.size(), VK_NULL_HANDLE);

		const auto& renderPass = GetRenderPass();

		for (uint32_t i = 0; i < m_FrameBufferHandles.size(); i++) {
			VkImageView swapChainView = m_SwapChainImageViews[i].GetVulkanHandle();

			VkFramebufferCreateInfo createInfo = VulkanAPI::FramebufferCreateInfo(renderPass->GetVulkanHandle(), 1, &swapChainView, m_CreateInfo.Width, m_CreateInfo.Height, 1);
			LUCY_VK_ASSERT(vkCreateFramebuffer(m_VulkanDevice->GetLogicalDevice(), &createInfo, nullptr, &m_FrameBufferHandles[i]));
		}
	}

	void VulkanSwapChainFrameBuffer::RTDestroyResource() {
		//no ownership for imageview is taken, the owner is the swapchain
		//so it should also get destroyed in the swapchain
			
		for (uint32_t i = 0; i < m_FrameBufferHandles.size(); i++) {
			if (m_FrameBufferHandles[i])
				vkDestroyFramebuffer(m_VulkanDevice->GetLogicalDevice(), m_FrameBufferHandles[i], nullptr);
			m_FrameBufferHandles[i] = VK_NULL_HANDLE;
		}
	}

	void VulkanSwapChainFrameBuffer::RTRecreate(uint32_t width, uint32_t height) {
		m_CreateInfo.Width = width;
		m_CreateInfo.Height = height;

		RTDestroyResource();
		CreateForSwapChain(); //m_SwapChainImageViews is still valid.
	}
}