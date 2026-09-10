#include "lypch.h"
#include "VulkanFrameBuffer.h"

#include "vulkan/vulkan.h"

#include "Renderer/Device/VulkanRenderDevice.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	VulkanFrameBuffer::VulkanFrameBuffer(const FrameBufferCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device)
		: FrameBuffer(createInfo), m_ImageHandles(m_CreateInfo.ImageBufferHandles), m_DepthImageHandles(m_CreateInfo.DepthImageHandles) {
		LUCY_ASSERT(!m_ImageHandles.empty() || !m_DepthImageHandles.empty(), "Imagebuffer and depth is empty!");
		
		RTCreate(device);
	}

	void VulkanFrameBuffer::RTCreate(const Ref<VulkanRenderDevice>& vulkanDevice) {
		const auto& renderPass = GetRenderPass();
		const uint32_t frameBufferCount = m_CreateInfo.IsInFlight ? Renderer::GetMaxFramesInFlight() : 1;

		m_FrameBufferHandles.resize(frameBufferCount, VK_NULL_HANDLE);

		for (uint32_t frameIndex = 0; frameIndex < frameBufferCount; frameIndex++) {
			std::vector<VkImageView> imageViewHandles;
			imageViewHandles.reserve(renderPass->GetColorAttachmentCount() + (renderPass->IsDepthBuffered() ? 1 : 0));

			for (const RenderDeviceResourceHandle& imageHandle : m_CreateInfo.ImageBufferHandles[frameIndex]) {
				const auto& image = vulkanDevice->AccessResource<VulkanImage>(imageHandle);
				imageViewHandles.emplace_back(image->GetImageView().GetVulkanHandle());
			}

			if (renderPass->IsDepthBuffered()) {
				const uint32_t depthIndex = m_CreateInfo.IsInFlight ? frameIndex : 0;
				imageViewHandles.push_back(GetDepthImage(depthIndex)->GetImageView().GetVulkanHandle());
			}

			VkFramebufferCreateInfo createInfo = VulkanAPI::FramebufferCreateInfo(renderPass->GetVulkanHandle(), (uint32_t)imageViewHandles.size(), 
				imageViewHandles.data(), m_CreateInfo.Width, m_CreateInfo.Height, 1);

			LUCY_VK_ASSERT(vkCreateFramebuffer(vulkanDevice->GetLogicalDevice(), &createInfo, nullptr, &m_FrameBufferHandles[frameIndex]));
		}
	}

	Ref<VulkanImage> VulkanFrameBuffer::GetImage(uint32_t frameIndex, uint32_t attachmentIndex) {
		return Renderer::AccessResource<VulkanImage>(m_ImageHandles[frameIndex][attachmentIndex]);
	}

	Ref<VulkanImage> VulkanFrameBuffer::GetDepthImage(uint32_t index) {
		return Renderer::AccessResource<VulkanImage>(m_DepthImageHandles[index]);
	}

	Ref<VulkanRenderPass> VulkanFrameBuffer::GetRenderPass() {
		return Renderer::AccessResource<VulkanRenderPass>(m_CreateInfo.RenderPassHandle);
	}

	void VulkanFrameBuffer::RTRecreate(uint32_t width, uint32_t height) {
		m_CreateInfo.Width = width;
		m_CreateInfo.Height = height;

		for (uint32_t frameIndex = 0; frameIndex < m_ImageHandles.size(); frameIndex++) {
			for (uint32_t attachmentIndex = 0; attachmentIndex < m_ImageHandles[frameIndex].size(); attachmentIndex++) {
				GetImage(frameIndex, attachmentIndex)->RTRecreate(width, height);
			}
		}

		for (uint32_t frameIndex = 0; frameIndex < m_DepthImageHandles.size(); frameIndex++)
			GetDepthImage(frameIndex)->RTRecreate(width, height);

		Renderer::EnqueueResourceRecreate([this](const Ref<RenderDevice>& device) -> RenderDeletionFunc {
			auto vulkanDevice = device->As<VulkanRenderDevice>();

			auto oldFrameBufferHandles = std::exchange(m_FrameBufferHandles, {});

			RTCreate(vulkanDevice);

			return [oldFrameBufferHandles = std::move(oldFrameBufferHandles)](const Ref<RenderDevice>& device) {
				auto vulkanDevice = device->As<VulkanRenderDevice>();

				for (auto frameBuffer : oldFrameBufferHandles) {
					if (frameBuffer)
						vkDestroyFramebuffer(vulkanDevice->GetLogicalDevice(), frameBuffer, nullptr);
				}
			};
		});
	}

	void VulkanFrameBuffer::RTDestroyResource(RenderDevice* device) {
		auto vulkanDevice = reinterpret_cast<VulkanRenderDevice*>(device);
		/*for (auto imageHandle : m_ImageHandles)
			vulkanDevice->RTDestroyResource(imageHandle);
		if (Renderer::IsValidRenderResource(m_DepthImageHandle))
			vulkanDevice->RTDestroyResource(m_DepthImageHandle);*/
		DestroyHandles(vulkanDevice);
	}

	void VulkanFrameBuffer::DestroyHandles(VulkanRenderDevice* device) {
		for (uint32_t i = 0; i < m_FrameBufferHandles.size(); i++) {
			if (m_FrameBufferHandles[i])
				vkDestroyFramebuffer(device->GetLogicalDevice(), m_FrameBufferHandles[i], nullptr);
			m_FrameBufferHandles[i] = VK_NULL_HANDLE;
		}
	}

	VulkanSwapChainFrameBuffer::VulkanSwapChainFrameBuffer(const Ref<VulkanRenderDevice>& vulkanDevice, const VkExtent2D& extent, const std::vector<VulkanImageView>& swapChainImageViews, const Ref<VulkanRenderPass>& renderPass)
		: FrameBuffer(FrameBufferCreateInfo{ .Width = extent.width, .Height = extent.height }), m_SwapChainImageViews(swapChainImageViews), m_RenderPass(renderPass) {
		LUCY_ASSERT(!m_SwapChainImageViews.empty(), "Swapchain image views are empty!");
		CreateForSwapChain(vulkanDevice);
	}

	void VulkanSwapChainFrameBuffer::CreateForSwapChain(const Ref<VulkanRenderDevice>& vulkanDevice) {
		m_FrameBufferHandles.resize(m_SwapChainImageViews.size(), VK_NULL_HANDLE);

		const auto& renderPass = GetRenderPass();

		for (uint32_t i = 0; i < m_FrameBufferHandles.size(); i++) {
			VkImageView swapChainView = m_SwapChainImageViews[i].GetVulkanHandle();

			VkFramebufferCreateInfo createInfo = VulkanAPI::FramebufferCreateInfo(renderPass->GetVulkanHandle(), 1, &swapChainView, m_CreateInfo.Width, m_CreateInfo.Height, 1);
			LUCY_VK_ASSERT(vkCreateFramebuffer(vulkanDevice->GetLogicalDevice(), &createInfo, nullptr, &m_FrameBufferHandles[i]));
		}
	}

	void VulkanSwapChainFrameBuffer::RTDestroyResource(RenderDevice* device) {
		//no ownership for imageview is taken, the owner is the swapchain
		//so it should also get destroyed in the swapchain
		
		auto vulkanDevice = reinterpret_cast<VulkanRenderDevice*>(device);
		for (uint32_t i = 0; i < m_FrameBufferHandles.size(); i++) {
			if (m_FrameBufferHandles[i])
				vkDestroyFramebuffer(vulkanDevice->GetLogicalDevice(), m_FrameBufferHandles[i], nullptr);
			m_FrameBufferHandles[i] = VK_NULL_HANDLE;
		}
	}

	void VulkanSwapChainFrameBuffer::RTRecreate(uint32_t width, uint32_t height) {
		m_CreateInfo.Width = width;
		m_CreateInfo.Height = height;

		Renderer::EnqueueResourceRecreate([this](const Ref<RenderDevice>& device) -> RenderDeletionFunc {
			auto vulkanDevice = device->As<VulkanRenderDevice>();

			auto oldFrameBufferHandles = std::exchange(m_FrameBufferHandles, {});

			CreateForSwapChain(vulkanDevice); //m_SwapChainImageViews is still valid.

			return [oldFrameBufferHandles = std::move(oldFrameBufferHandles)](const Ref<RenderDevice>& device) {
				auto vulkanDevice = device->As<VulkanRenderDevice>();

				for (auto frameBuffer : oldFrameBufferHandles) {
					if (frameBuffer)
						vkDestroyFramebuffer(vulkanDevice->GetLogicalDevice(), frameBuffer, nullptr);
				}
			};
		});
	}
}