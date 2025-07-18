#include "lypch.h"

#include "VulkanSwapChain.h"

#include "Renderer/Synchronization/VulkanSyncItems.h"
#include "Renderer/Device/VulkanRenderDevice.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	void VulkanSwapChain::Init() {
		m_SwapChain = Create(nullptr);
	}

	VkSwapchainKHR VulkanSwapChain::Create(VkSwapchainKHR oldSwapChain) {
		const auto& vulkanDevice = GetRenderDevice()->As<VulkanRenderDevice>();
		VkPhysicalDevice physicalDevice = vulkanDevice->GetPhysicalDevice();
		VkDevice logicalDevice = vulkanDevice->GetLogicalDevice();

		SwapChainCapabilities capabilities = GetSwapChainCapabilities(physicalDevice);
		m_SelectedFormat = ChooseSwapSurfaceFormat(capabilities);
		m_SelectedPresentMode = ChooseSwapPresentMode(capabilities);
		m_SelectedSwapExtent = ChooseSwapExtent(capabilities);

		uint32_t imageCount = capabilities.surfaceCapabilities.minImageCount + 1;
		if (capabilities.surfaceCapabilities.maxImageCount > 0 && imageCount > capabilities.surfaceCapabilities.maxImageCount) {
			imageCount = capabilities.surfaceCapabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo = VulkanAPI::SwapchainCreateInfo(this, imageCount, oldSwapChain, capabilities.surfaceCapabilities.currentTransform);

		VkSwapchainKHR swapChain;
		LUCY_VK_ASSERT(vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapChain));

		if (oldSwapChain) //for resize
			vkDestroySwapchainKHR(vulkanDevice->GetLogicalDevice(), m_OldSwapChain, nullptr);

		uint32_t swapChainImageCount = 0;
		vkGetSwapchainImagesKHR(logicalDevice, swapChain, &swapChainImageCount, nullptr);

		m_SwapChainImages.resize(swapChainImageCount);
		vkGetSwapchainImagesKHR(logicalDevice, swapChain, &swapChainImageCount, m_SwapChainImages.data());

		if (oldSwapChain) { //if resized
			for (uint32_t i = 0; i < swapChainImageCount; i++) {
				ImageViewCreateInfo imageViewCreateInfo;
				imageViewCreateInfo.Image = m_SwapChainImages[i];
				imageViewCreateInfo.ImageType = ImageType::Type2D;
				imageViewCreateInfo.Format = m_SelectedFormat.format;

				m_SwapChainImageViews[i].RTRecreate(imageViewCreateInfo);
			}
		} else { //initial startup
			m_SwapChainImageViews.reserve(swapChainImageCount);

			for (uint32_t i = 0; i < swapChainImageCount; i++) {
				ImageViewCreateInfo imageViewCreateInfo;
				imageViewCreateInfo.Image = m_SwapChainImages[i];
				imageViewCreateInfo.ImageType = ImageType::Type2D;
				imageViewCreateInfo.Format = m_SelectedFormat.format;

				m_SwapChainImageViews.emplace_back(imageViewCreateInfo, vulkanDevice);
			}

			RenderPassCreateInfo renderPassCreateInfo;
			renderPassCreateInfo.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
			renderPassCreateInfo.Layout.ColorAttachments = {
				RenderPassLayout::Attachment {
					.Format = GetLucyImageFormat(GetSurfaceFormat().format),
					.LoadStoreOperation = RenderPassLoadStoreAttachments::ClearNone,
					.StencilLoadStoreOperation = RenderPassLoadStoreAttachments::DontCareDontCare,
					.Initial = VK_IMAGE_LAYOUT_UNDEFINED,
					.Final = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
					.Reference {
						.Layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
					},
				}
			};

			m_SwapChainRenderPass = Memory::CreateRef<VulkanRenderPass>(renderPassCreateInfo, vulkanDevice);
			m_SwapChainFrameBuffer = Memory::CreateUnique<VulkanSwapChainFrameBuffer>(vulkanDevice, m_SelectedSwapExtent, m_SwapChainImageViews, m_SwapChainRenderPass);
		}

		return swapChain;
	}

	VulkanSwapChain::VulkanSwapChain(const Ref<Window>& window, const Ref<RenderDevice>& renderDevice) 
		: SwapChain(window, renderDevice) {
	}

	void VulkanSwapChain::Recreate() {
		m_OldSwapChain = m_SwapChain;
		m_SwapChain = Create(m_OldSwapChain);

		auto [width, height] = GetExtent();
		m_SwapChainFrameBuffer->RTRecreate(width, height);
	}

	RenderContextResultCodes VulkanSwapChain::AcquireNextImage(const Semaphore& currentFrameImageAvailSemaphore, uint32_t& imageIndex) {
		LUCY_PROFILE_NEW_EVENT("VulkanSwapChain::AcquireNextImage");
		const auto& vulkanDevice = GetRenderDevice()->As<VulkanRenderDevice>();

		VkResult result = vkAcquireNextImageKHR(vulkanDevice->GetLogicalDevice(), m_SwapChain, UINT64_MAX, currentFrameImageAvailSemaphore.GetSemaphore(), VK_NULL_HANDLE, &imageIndex);
		return (RenderContextResultCodes)result;
	}

	RenderContextResultCodes VulkanSwapChain::Present(const Semaphore& signalSemaphore, uint32_t& imageIndex) {
		LUCY_PROFILE_NEW_EVENT("VulkanSwapChain::Present");
		const auto& vulkanDevice = GetRenderDevice()->As<VulkanRenderDevice>();

		VkPresentInfoKHR presentInfo = VulkanAPI::PresentInfoKHR(1, &m_SwapChain, &imageIndex, 1, &signalSemaphore.GetSemaphore());
		VkResult queuePresentResult = vkQueuePresentKHR(vulkanDevice->GetPresentQueue(), &presentInfo);

		return (RenderContextResultCodes)queuePresentResult;
	}

	SwapChainCapabilities VulkanSwapChain::GetSwapChainCapabilities(VkPhysicalDevice device) const {
		SwapChainCapabilities capabilities;
		VkSurfaceKHR surface = GetWindow()->GetVulkanSurface();

		LUCY_VK_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities.surfaceCapabilities));

		uint32_t formatsCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatsCount, nullptr);
		capabilities.formats.resize(formatsCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatsCount, capabilities.formats.data());

		uint32_t presentModesCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &presentModesCount, nullptr);
		capabilities.presentModes.resize(presentModesCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModesCount, capabilities.presentModes.data());

		LUCY_ASSERT(!capabilities.presentModes.empty() || !capabilities.formats.empty(), "Graphics card does not support swap chain formats/present modes");
		return capabilities;
	}

	VkSurfaceFormatKHR VulkanSwapChain::ChooseSwapSurfaceFormat(const SwapChainCapabilities& capabilities) const {
		for (const auto& availableFormat : capabilities.formats) {
			if (availableFormat.format == VK_FORMAT_R8G8B8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}
		return capabilities.formats[0];
	}

	VkPresentModeKHR VulkanSwapChain::ChooseSwapPresentMode(const SwapChainCapabilities& capabilities) const {
		for (const auto& availablePresentMode : capabilities.presentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}
		return VK_PRESENT_MODE_FIFO_KHR; //guaranteed to support
	}

	VkExtent2D VulkanSwapChain::ChooseSwapExtent(const SwapChainCapabilities& capabilities) const {
		if (capabilities.surfaceCapabilities.currentExtent.width != UINT32_MAX) {
			return capabilities.surfaceCapabilities.currentExtent;
		} else {
			int32_t width, height;
			glfwGetFramebufferSize(GetWindow()->Raw(), &width, &height);

			VkExtent2D actualExtent = { (uint32_t)width, (uint32_t)height };

			actualExtent.width = std::clamp(actualExtent.width, capabilities.surfaceCapabilities.minImageExtent.width, capabilities.surfaceCapabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.surfaceCapabilities.minImageExtent.height, capabilities.surfaceCapabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	void VulkanSwapChain::Destroy() {
		LUCY_ASSERT(Renderer::IsOnRenderThread());

		for (uint32_t i = 0; i < m_SwapChainImageViews.size(); i++)
			m_SwapChainImageViews[i].RTDestroyResource();
		m_SwapChainFrameBuffer->RTDestroyResource();
		m_SwapChainRenderPass->RTDestroyResource();

		const auto& vulkanDevice = GetRenderDevice()->As<VulkanRenderDevice>();
		vkDestroySwapchainKHR(vulkanDevice->GetLogicalDevice(), m_SwapChain, nullptr);
	}
}