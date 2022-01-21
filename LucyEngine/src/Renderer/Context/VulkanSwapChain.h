#pragma once

#include "vulkan/vulkan.h"

namespace Lucy {

	struct SwapChainCapabilities {
		VkSurfaceCapabilitiesKHR surfaceCapabilities{};
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	class VulkanSwapChain {
	private:
		VulkanSwapChain() = default;
	public:
		~VulkanSwapChain() = default;

		static VulkanSwapChain& Get();

		void Create();
		void Recreate();
		VkResult AcquireNextImage(VkSemaphore currentFrameImageAvailSemaphore, uint32_t& imageIndex);
		void Destroy();

		inline VkExtent2D& GetExtent() { return m_SelectedSwapExtent; }
		inline VkSurfaceFormatKHR& GetSurfaceFormat() { return m_SelectedFormat; }
		inline VkSwapchainKHR GetVulkanHandle() { return m_SwapChain; }
		inline uint32_t GetImageCount() { return m_SwapChainImages.size(); }
	private:
		SwapChainCapabilities GetSwapChainCapabilities(VkPhysicalDevice device);
		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(SwapChainCapabilities& capabilities);
		VkPresentModeKHR ChooseSwapPresentMode(SwapChainCapabilities& capabilities);
		VkExtent2D ChooseSwapExtent(SwapChainCapabilities& capabilities);

		VkSwapchainKHR m_SwapChain;
		
		std::vector<VkImage> m_SwapChainImages;
		std::vector<VkImageView> m_SwapChainImageViews;
		
		VkSurfaceFormatKHR m_SelectedFormat;
		VkPresentModeKHR m_SelectedPresentMode;
		VkExtent2D m_SelectedSwapExtent;

		friend class VulkanFrameBuffer;
		friend class VulkanRenderCommand;
	};
}