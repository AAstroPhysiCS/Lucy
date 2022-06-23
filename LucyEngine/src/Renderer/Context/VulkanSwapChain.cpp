#include "lypch.h"
#include "VulkanSwapChain.h"

#include "vulkan/vulkan.h"
#include "VulkanDevice.h"

#include "Renderer/Memory/Buffer/Vulkan/VulkanFrameBuffer.h"
#include "Renderer/Renderer.h"
#include "Renderer/CommandQueue.h"

namespace Lucy {

	VulkanSwapChain::VulkanSwapChain() {
		m_WaitSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_SignalSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	}

	VulkanSwapChain& VulkanSwapChain::Get() {
		static VulkanSwapChain s_Instance;
		return s_Instance;
	}

	void VulkanSwapChain::Create() {
		m_SwapChain = Create(nullptr);
	}

	VkSwapchainKHR VulkanSwapChain::Create(VkSwapchainKHR oldSwapChain) {
		const VulkanDevice& device = VulkanDevice::Get();

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
		createInfo.surface = Renderer::GetWindow()->GetVulkanSurface();
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = m_SelectedFormat.format;
		createInfo.imageColorSpace = m_SelectedFormat.colorSpace;
		createInfo.imageExtent = m_SelectedSwapExtent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
		createInfo.preTransform = capabilities.surfaceCapabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = m_SelectedPresentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = oldSwapChain;

		VkSwapchainKHR swapChain;
		LUCY_VK_ASSERT(vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapChain));

		if (oldSwapChain) //for resize
			vkDestroySwapchainKHR(device.GetLogicalDevice(), m_OldSwapChain, nullptr);

		uint32_t swapChainImageCount = 0;
		vkGetSwapchainImagesKHR(logicalDevice, swapChain, &swapChainImageCount, nullptr);

		m_SwapChainImages.resize(swapChainImageCount);
		vkGetSwapchainImagesKHR(logicalDevice, swapChain, &swapChainImageCount, m_SwapChainImages.data());

		if (oldSwapChain) { //if resized
			for (uint32_t i = 0; i < swapChainImageCount; i++) {
				ImageViewSpecification specs;
				specs.Image = m_SwapChainImages[i];
				specs.Format = m_SelectedFormat.format;
				specs.ViewType = VK_IMAGE_VIEW_TYPE_2D;

				m_SwapChainImageViews[i].Recreate(specs);
			}
		} else { //initial startup
			m_SwapChainImageViews.reserve(swapChainImageCount);

			for (uint32_t i = 0; i < swapChainImageCount; i++) {
				ImageViewSpecification specs;
				specs.Image = m_SwapChainImages[i];
				specs.Format = m_SelectedFormat.format;
				specs.ViewType = VK_IMAGE_VIEW_TYPE_2D;

				m_SwapChainImageViews.emplace_back(specs);
			}
		}

		m_SwapChainFrameBufferDesc->ImageViews = m_SwapChainImageViews;

		return swapChain;
	}

	void VulkanSwapChain::Recreate() {
		const VulkanDevice& device = VulkanDevice::Get();
		LUCY_VK_ASSERT(vkDeviceWaitIdle(device.GetLogicalDevice()));

		m_OldSwapChain = m_SwapChain;
		m_SwapChain = Create(m_OldSwapChain);
	}

	void VulkanSwapChain::BeginFrame() {
		const auto& device = VulkanDevice::Get();
		VkDevice deviceVulkanHandle = device.GetLogicalDevice();

		vkWaitForFences(deviceVulkanHandle, 1, &m_InFlightFences[m_CurrentFrameIndex].GetFence(), VK_TRUE, UINT64_MAX);
		vkResetFences(deviceVulkanHandle, 1, &m_InFlightFences[m_CurrentFrameIndex].GetFence());

		m_LastSwapChainResult = AcquireNextImage(m_WaitSemaphores[m_CurrentFrameIndex].GetSemaphore(), m_ImageIndex);
		if (m_LastSwapChainResult == VK_ERROR_OUT_OF_DATE_KHR || m_LastSwapChainResult == VK_SUBOPTIMAL_KHR) {
			return;
		}
	}

	void VulkanSwapChain::EndFrame(const CommandQueue& commandQueue) {
		if (m_LastSwapChainResult == VK_ERROR_OUT_OF_DATE_KHR || m_LastSwapChainResult == VK_SUBOPTIMAL_KHR)
			return;
		SubmitToQueue(commandQueue.GetCurrentCommandBuffer());
	}

	VkResult VulkanSwapChain::AcquireNextImage(VkSemaphore currentFrameImageAvailSemaphore, uint32_t& imageIndex) {
		VkDevice deviceVulkanHandle = VulkanDevice::Get().GetLogicalDevice();
		return vkAcquireNextImageKHR(deviceVulkanHandle, m_SwapChain, UINT64_MAX, currentFrameImageAvailSemaphore, VK_NULL_HANDLE, &imageIndex);
	}

	VkResult VulkanSwapChain::Present() {
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &m_SignalSemaphores[m_CurrentFrameIndex].GetSemaphore();

		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_SwapChain;
		presentInfo.pImageIndices = &m_ImageIndex;
		presentInfo.pResults = nullptr;

		const auto& device = VulkanDevice::Get();
		m_LastSwapChainResult = vkQueuePresentKHR(device.GetPresentQueue(), &presentInfo);

		m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;

		return m_LastSwapChainResult;
	}

	SwapChainCapabilities VulkanSwapChain::GetSwapChainCapabilities(VkPhysicalDevice device) {
		SwapChainCapabilities capabilities;
		VkSurfaceKHR surface = Renderer::GetWindow()->GetVulkanSurface();

		LUCY_VK_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities.surfaceCapabilities));

		uint32_t formatsCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatsCount, nullptr);
		capabilities.formats.resize(formatsCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatsCount, capabilities.formats.data());

		uint32_t presentModesCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &presentModesCount, nullptr);
		capabilities.presentModes.resize(presentModesCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModesCount, capabilities.presentModes.data());

		if (capabilities.presentModes.empty() || capabilities.formats.empty()) {
			LUCY_CRITICAL("Graphics card does not support swap chain formats/present modes");
			LUCY_ASSERT(false);
		}
		return capabilities;
	}

	VkSurfaceFormatKHR VulkanSwapChain::ChooseSwapSurfaceFormat(const SwapChainCapabilities& capabilities) {
		for (const auto& availableFormat : capabilities.formats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}
		return capabilities.formats[0];
	}

	VkPresentModeKHR VulkanSwapChain::ChooseSwapPresentMode(const SwapChainCapabilities& capabilities) {
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
			glfwGetFramebufferSize(Renderer::GetWindow()->Raw(), &width, &height);

			VkExtent2D actualExtent = { width, height };

			actualExtent.width = std::clamp(actualExtent.width, capabilities.surfaceCapabilities.minImageExtent.width, capabilities.surfaceCapabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.surfaceCapabilities.minImageExtent.height, capabilities.surfaceCapabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	void VulkanSwapChain::Destroy() {
		const VulkanDevice& device = VulkanDevice::Get();

		for (uint32_t i = 0; i < m_SwapChainImageViews.size(); i++)
			m_SwapChainImageViews[i].Destroy();

		vkDestroySwapchainKHR(device.GetLogicalDevice(), m_SwapChain, nullptr);

		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			m_WaitSemaphores[i].Destroy();
			m_SignalSemaphores[i].Destroy();
			m_InFlightFences[i].Destroy();
		}
	}

	void VulkanSwapChain::SubmitToQueue(VkCommandBuffer commandBuffer) {
		VulkanDevice& device = VulkanDevice::Get();

		VkFence currentFrameFence = m_InFlightFences[m_CurrentFrameIndex].GetFence();
		VkSemaphore currentFrameWaitSemaphore = m_WaitSemaphores[m_CurrentFrameIndex].GetSemaphore(); // image is available, image is renderable
		VkSemaphore currentFrameSignalSemaphore = m_SignalSemaphores[m_CurrentFrameIndex].GetSemaphore(); // rendering finished, signal it

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkPipelineStageFlags imageWaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &currentFrameWaitSemaphore;
		submitInfo.pWaitDstStageMask = imageWaitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &currentFrameSignalSemaphore;

		LUCY_VK_ASSERT(vkQueueSubmit(device.GetGraphicsQueue(), 1, &submitInfo, currentFrameFence));
		vkWaitForFences(device.GetLogicalDevice(), 1, &currentFrameFence, VK_TRUE, UINT64_MAX);
	}
}