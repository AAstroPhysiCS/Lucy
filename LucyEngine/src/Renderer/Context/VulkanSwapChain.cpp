#include "lypch.h"
#include "VulkanSwapChain.h"

#include "vulkan/vulkan.h"
#include "VulkanDevice.h"
#include "Renderer/Renderer.h"

namespace Lucy {

	VulkanSwapChain& VulkanSwapChain::Get() {
		static VulkanSwapChain s_Instance;
		return s_Instance;
	}

	void VulkanSwapChain::Create() {
		VulkanDevice& device = VulkanDevice::Get();

		VkPhysicalDevice physicalDevice = device.GetPhysicalDevice();
		VkDevice logicalDevice = device.GetLogicalDevice();

		SwapChainCapabilities capabilities = GetSwapChainCapabilities(physicalDevice);
		m_SelectedFormat = ChooseSwapSurfaceFormat(capabilities);
		m_SelectedPresentMode = ChooseSwapPresentMode(capabilities);
		m_SelectedSwapExtent = ChooseSwapExtent(capabilities);

		uint32_t imageCount = capabilities.surfaceCapabilities.minImageCount + 1;
		if (capabilities.surfaceCapabilities.maxImageCount > 0 && imageCount > capabilities.surfaceCapabilities.maxImageCount) {
			imageCount = capabilities.surfaceCapabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = Renderer::s_Window->m_Surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = m_SelectedFormat.format;
		createInfo.imageColorSpace = m_SelectedFormat.colorSpace;
		createInfo.imageExtent = m_SelectedSwapExtent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		//See tutorial for the most optimum setting
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
		createInfo.preTransform = capabilities.surfaceCapabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = m_SelectedPresentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		LUCY_VULKAN_ASSERT(vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &m_SwapChain));

		uint32_t swapChainImageCount = 0;
		vkGetSwapchainImagesKHR(logicalDevice, m_SwapChain, &swapChainImageCount, nullptr);

		m_SwapChainImages.resize(swapChainImageCount);
		vkGetSwapchainImagesKHR(logicalDevice, m_SwapChain, &swapChainImageCount, m_SwapChainImages.data());

		m_SwapChainImageViews.resize(swapChainImageCount);
		for (uint32_t i = 0; i < m_SwapChainImageViews.size(); i++) {
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = m_SwapChainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = m_SelectedFormat.format;

			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			LUCY_VULKAN_ASSERT(vkCreateImageView(logicalDevice, &createInfo, nullptr, &m_SwapChainImageViews[i]));
		}
	}

	void VulkanSwapChain::Recreate() {
		Destroy();
		Create();
	}

	VkResult VulkanSwapChain::AcquireNextImage(VkSemaphore currentFrameImageAvailSemaphore, uint32_t& imageIndex) {
		VkDevice deviceVulkanHandle = VulkanDevice::Get().GetLogicalDevice();
		return vkAcquireNextImageKHR(deviceVulkanHandle, m_SwapChain, UINT64_MAX, currentFrameImageAvailSemaphore, VK_NULL_HANDLE, &imageIndex);
	}

	SwapChainCapabilities VulkanSwapChain::GetSwapChainCapabilities(VkPhysicalDevice device) {
		SwapChainCapabilities capabilities;
		LUCY_VULKAN_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, Renderer::s_Window->m_Surface, &capabilities.surfaceCapabilities));

		uint32_t formatsCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, Renderer::s_Window->m_Surface, &formatsCount, nullptr);
		capabilities.formats.resize(formatsCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, Renderer::s_Window->m_Surface, &formatsCount, capabilities.formats.data());

		uint32_t presentModesCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, Renderer::s_Window->m_Surface, &presentModesCount, nullptr);
		capabilities.presentModes.resize(presentModesCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, Renderer::s_Window->m_Surface, &presentModesCount, capabilities.presentModes.data());

		if (capabilities.presentModes.empty() || capabilities.formats.empty()) {
			LUCY_CRITICAL("Graphics card does not support swap chain formats/present modes");
			LUCY_ASSERT(false);
		}
		return capabilities;
	}

	VkSurfaceFormatKHR VulkanSwapChain::ChooseSwapSurfaceFormat(SwapChainCapabilities& capabilities) {
		for (const auto& availableFormat : capabilities.formats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return capabilities.formats[0];
	}

	VkPresentModeKHR VulkanSwapChain::ChooseSwapPresentMode(SwapChainCapabilities& capabilities) {
		for (const auto& availablePresentMode : capabilities.presentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR; //guaranteed to support
	}

	VkExtent2D VulkanSwapChain::ChooseSwapExtent(SwapChainCapabilities& capabilities) {
		if (capabilities.surfaceCapabilities.currentExtent.width != UINT32_MAX) {
			return capabilities.surfaceCapabilities.currentExtent;
		} else {
			int32_t width, height;
			glfwGetFramebufferSize(Renderer::s_Window->Raw(), &width, &height);

			VkExtent2D actualExtent = { width, height };

			actualExtent.width = std::clamp(actualExtent.width, capabilities.surfaceCapabilities.minImageExtent.width, capabilities.surfaceCapabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.surfaceCapabilities.minImageExtent.height, capabilities.surfaceCapabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	void VulkanSwapChain::Destroy() {
		VulkanDevice& device = VulkanDevice::Get();
		for (uint32_t i = 0; i < m_SwapChainImageViews.size(); i++) {
			vkDestroyImageView(device.GetLogicalDevice(), m_SwapChainImageViews[i], nullptr);
		}
		vkDestroySwapchainKHR(device.GetLogicalDevice(), m_SwapChain, nullptr);
	}
}