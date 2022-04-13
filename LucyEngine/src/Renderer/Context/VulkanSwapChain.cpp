#include "lypch.h"
#include "VulkanSwapChain.h"

#include "vulkan/vulkan.h"
#include "VulkanDevice.h"
#include "../RenderQueue.h"
#include "Renderer/Buffer/Vulkan/VulkanFrameBuffer.h"
#include "Renderer/Renderer.h"

namespace Lucy {

	VulkanSwapChain& VulkanSwapChain::Get() {
		static VulkanSwapChain s_Instance;
		return s_Instance;
	}

	void VulkanSwapChain::AfterInitialization() {
		m_ImageIsAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_RenderIsFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		m_CommandPool = VulkanCommandPool::Create({ VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, VK_COMMAND_BUFFER_LEVEL_PRIMARY, MAX_FRAMES_IN_FLIGHT });

		m_FirstInitialized = true;
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
		createInfo.oldSwapchain = oldSwapChain;

		VkSwapchainKHR swapChain;
		LUCY_VK_ASSERT(vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapChain));

		if (oldSwapChain) { //for resize
			for (uint32_t i = 0; i < m_SwapChainImageViews.size(); i++)
				m_SwapChainImageViews[i].Destroy();
			vkDestroySwapchainKHR(device.GetLogicalDevice(), m_OldSwapChain, nullptr);
		}

		uint32_t swapChainImageCount = 0;
		vkGetSwapchainImagesKHR(logicalDevice, swapChain, &swapChainImageCount, nullptr);

		m_SwapChainImages.resize(swapChainImageCount);
		vkGetSwapchainImagesKHR(logicalDevice, swapChain, &swapChainImageCount, m_SwapChainImages.data());

		m_SwapChainImageViews.reserve(swapChainImageCount);

		for (uint32_t i = 0; i < swapChainImageCount; i++) {
			ImageViewSpecification specs;
			specs.Image = m_SwapChainImages[i];
			specs.Format = m_SelectedFormat.format;
			specs.ViewType = VK_IMAGE_VIEW_TYPE_2D;

			m_SwapChainImageViews.emplace_back(specs);
		}

		if (!m_FirstInitialized)
			AfterInitialization();

		return swapChain;
	}

	void VulkanSwapChain::Recreate() {
		const VulkanDevice& device = VulkanDevice::Get();
		vkDeviceWaitIdle(device.GetLogicalDevice());

		m_OldSwapChain = m_SwapChain;
		m_SwapChain = Create(m_OldSwapChain);

		m_CommandPool->Recreate();
	}

	void VulkanSwapChain::BeginFrame() {
		const auto& device = VulkanDevice::Get();
		VkDevice deviceVulkanHandle = device.GetLogicalDevice();

		vkWaitForFences(deviceVulkanHandle, 1, &m_InFlightFences[s_CurrentFrameIndex].GetFence(), VK_TRUE, UINT64_MAX);
		vkResetFences(deviceVulkanHandle, 1, &m_InFlightFences[s_CurrentFrameIndex].GetFence());

		m_LastSwapChainResult = AcquireNextImage(m_ImageIsAvailableSemaphores[s_CurrentFrameIndex].GetSemaphore(), s_ImageIndex);
		if (m_LastSwapChainResult == VK_ERROR_OUT_OF_DATE_KHR || m_LastSwapChainResult == VK_SUBOPTIMAL_KHR) {
			return;
		}
	}

	void VulkanSwapChain::Execute(const RenderCommandQueue& renderCommandQueue) {
		if (m_LastSwapChainResult == VK_ERROR_OUT_OF_DATE_KHR || m_LastSwapChainResult == VK_SUBOPTIMAL_KHR)
			return;

		auto commandBuffer = m_CommandPool->GetCommandBuffer(s_CurrentFrameIndex);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		LUCY_VK_ASSERT(vkBeginCommandBuffer(commandBuffer, &beginInfo));

		VkViewport viewport;
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = m_SelectedSwapExtent.width;
		viewport.height = m_SelectedSwapExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = m_SelectedSwapExtent;

		if (viewport.width == 0 || viewport.height == 0) return;

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		for (const RenderCommand& renderCommand : renderCommandQueue.GetCommandQueue()) {
			if (!renderCommand.Pipeline) { //meaning that the function provides a pipeline already
				renderCommand.Func(commandBuffer);
			} else {
				//organizing all "distinct!" sets
				const std::vector<VulkanDescriptorSet>& allSetsToBind = renderCommand.Pipeline->GetIndividualSetsToBind();
				std::vector<VkDescriptorSet> descriptorSetsToBind;
				for (VulkanDescriptorSet descriptorSet : allSetsToBind) {
					descriptorSetsToBind.push_back(descriptorSet.GetSetBasedOffCurrentFrame(s_CurrentFrameIndex));
				}

				auto& renderPass = renderCommand.Pipeline->GetRenderPass();
				auto& framebuffer = As(renderCommand.Pipeline->GetFrameBuffer(), VulkanFrameBuffer)->GetVulkanHandles();

				RenderPassBeginInfo beginInfo;
				beginInfo.CommandBuffer = commandBuffer;
				beginInfo.VulkanFrameBuffer = framebuffer[s_ImageIndex];
				renderPass->Begin(beginInfo);

				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderCommand.Pipeline->GetVulkanHandle());
				if (descriptorSetsToBind.size() != 0)
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderCommand.Pipeline->GetPipelineLayout(), 0, descriptorSetsToBind.size(), descriptorSetsToBind.data(), 0, nullptr);
				
				renderCommand.Func(commandBuffer);

				RenderPassEndInfo endInfo;
				endInfo.CommandBuffer = commandBuffer;
				renderPass->End(endInfo);
			}
		}

		LUCY_VK_ASSERT(vkEndCommandBuffer(commandBuffer));
	}

	void VulkanSwapChain::EndFrame() {
		if (m_LastSwapChainResult == VK_ERROR_OUT_OF_DATE_KHR)
			return;

		const auto& device = VulkanDevice::Get();
		VkDevice deviceVulkanHandle = device.GetLogicalDevice();

		VkFence currentFrameFence = m_InFlightFences[s_CurrentFrameIndex].GetFence();
		VkSemaphore currentFrameImageAvailSemaphore = m_ImageIsAvailableSemaphores[s_CurrentFrameIndex].GetSemaphore();
		VkSemaphore currentFrameRenderFinishedSemaphore = m_RenderIsFinishedSemaphores[s_CurrentFrameIndex].GetSemaphore();

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkPipelineStageFlags imageWaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &currentFrameImageAvailSemaphore;
		submitInfo.pWaitDstStageMask = imageWaitStages;

		VkCommandBuffer targetedCommandBuffer = m_CommandPool->GetCommandBuffer(s_CurrentFrameIndex);
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &targetedCommandBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &currentFrameRenderFinishedSemaphore;

		LUCY_VK_ASSERT(vkQueueSubmit(device.GetGraphicsQueue(), 1, &submitInfo, currentFrameFence));
		vkWaitForFences(deviceVulkanHandle, 1, &currentFrameFence, VK_TRUE, UINT64_MAX);
	}

	VkResult VulkanSwapChain::AcquireNextImage(VkSemaphore currentFrameImageAvailSemaphore, uint32_t& imageIndex) {
		VkDevice deviceVulkanHandle = VulkanDevice::Get().GetLogicalDevice();
		return vkAcquireNextImageKHR(deviceVulkanHandle, m_SwapChain, UINT64_MAX, currentFrameImageAvailSemaphore, VK_NULL_HANDLE, &imageIndex);
	}

	VkResult VulkanSwapChain::Present() {
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &m_RenderIsFinishedSemaphores[s_CurrentFrameIndex].GetSemaphore();

		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_SwapChain;
		presentInfo.pImageIndices = &s_ImageIndex;
		presentInfo.pResults = nullptr;

		const auto& device = VulkanDevice::Get();
		m_LastSwapChainResult = vkQueuePresentKHR(device.GetPresentQueue(), &presentInfo);

		s_CurrentFrameIndex = (s_CurrentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;

		return m_LastSwapChainResult;
	}

	SwapChainCapabilities VulkanSwapChain::GetSwapChainCapabilities(VkPhysicalDevice device) {
		SwapChainCapabilities capabilities;
		LUCY_VK_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, Renderer::s_Window->m_Surface, &capabilities.surfaceCapabilities));

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
			glfwGetFramebufferSize(Renderer::s_Window->Raw(), &width, &height);

			VkExtent2D actualExtent = { width, height };

			actualExtent.width = std::clamp(actualExtent.width, capabilities.surfaceCapabilities.minImageExtent.width, capabilities.surfaceCapabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.surfaceCapabilities.minImageExtent.height, capabilities.surfaceCapabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	VkCommandBuffer VulkanSwapChain::BeginSingleTimeCommand() {
		return m_CommandPool->BeginSingleTimeCommand();
	}

	void VulkanSwapChain::EndSingleTimeCommand(VkCommandBuffer commandBuffer) {
		m_CommandPool->EndSingleTimeCommand(commandBuffer);
	}

	void VulkanSwapChain::Destroy() {
		const VulkanDevice& device = VulkanDevice::Get();
		m_CommandPool->Destroy();

		for (uint32_t i = 0; i < m_SwapChainImageViews.size(); i++)
			m_SwapChainImageViews[i].Destroy();

		vkDestroySwapchainKHR(device.GetLogicalDevice(), m_SwapChain, nullptr);

		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			m_ImageIsAvailableSemaphores[i].Destroy();
			m_RenderIsFinishedSemaphores[i].Destroy();
			m_InFlightFences[i].Destroy();
		}
	}
}