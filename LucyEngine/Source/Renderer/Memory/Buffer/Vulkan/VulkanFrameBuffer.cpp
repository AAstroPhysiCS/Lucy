#include "lypch.h"
#include "VulkanFrameBuffer.h"

#include "vulkan/vulkan.h"

#include "Renderer/Device/VulkanRenderDevice.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	VulkanFrameBuffer::VulkanFrameBuffer(const FrameBufferCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device)
		: FrameBuffer(createInfo), m_ImageHandles(m_CreateInfo.ImageBufferHandles), m_DepthImageHandle(createInfo.DepthImageHandle) {
		LUCY_ASSERT(!m_ImageHandles.empty() || m_DepthImageHandle, "Imagebuffer and depth is empty!");
		
		RTCreate(device);
	}

	void VulkanFrameBuffer::RTCreate(const Ref<VulkanRenderDevice>& vulkanDevice) {
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
					m_ImageHandles.push_back(vulkanDevice->CreateImage(Renderer::AccessResource<VulkanImage2D>(m_ImageHandles[i])));
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
				for (uint32_t j = 0; j < m_ImageHandles.size(); j++) {
					auto& view = GetImage(j)->GetImageView();
					imageViewHandles.push_back(view.GetVulkanHandle());
				}
			}

			if (renderPass->IsDepthBuffered())
				imageViewHandles.push_back(GetDepthImage()->GetImageView().GetVulkanHandle());

			VkFramebufferCreateInfo createInfo = VulkanAPI::FramebufferCreateInfo(renderPass->GetVulkanHandle(), (uint32_t)imageViewHandles.size(), imageViewHandles.data(), m_CreateInfo.Width, m_CreateInfo.Height, 1);
			LUCY_VK_ASSERT(vkCreateFramebuffer(vulkanDevice->GetLogicalDevice(), &createInfo, nullptr, &m_FrameBufferHandles[i]));
		}
	}

	Ref<VulkanImage> VulkanFrameBuffer::GetImage(uint32_t index) {
		return Renderer::AccessResource<VulkanImage>(m_ImageHandles[index]);
	}

	Ref<VulkanImage> VulkanFrameBuffer::GetDepthImage() {
		return Renderer::AccessResource<VulkanImage>(m_DepthImageHandle);
	}

	Ref<VulkanRenderPass> VulkanFrameBuffer::GetRenderPass() {
		return Renderer::AccessResource<VulkanRenderPass>(m_CreateInfo.RenderPassHandle);
	}

	void VulkanFrameBuffer::RTRecreate(uint32_t width, uint32_t height) {
		m_CreateInfo.Width = width;
		m_CreateInfo.Height = height;

		for (uint32_t i = 0; i < m_ImageHandles.size(); i++)
			GetImage(i)->RTRecreate(width, height);

		if (Renderer::IsValidRenderResource(m_DepthImageHandle))
			GetDepthImage()->RTRecreate(width, height);

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