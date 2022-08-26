#pragma once

#include "vulkan/vulkan.h"
#include "Renderer/Image/VulkanImage.h"

namespace Lucy {

	class Window;
	class CommandQueue;

	class Semaphore;
	class Fence;

	struct SwapChainCapabilities {
		VkSurfaceCapabilitiesKHR surfaceCapabilities{};
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	class VulkanSwapChain {
	public:
		~VulkanSwapChain() = default;
		static VulkanSwapChain& Get();

		void Create(Ref<Window>& window);
		void Recreate();
		VkResult AcquireNextImage(VkSemaphore currentFrameImageAvailSemaphore, uint32_t& imageIndex);
		VkResult Present(const Semaphore& signalSemaphore, uint32_t& imageIndex);

		/// <param name="currentFrameWaitSemaphore: image is available, image is renderable"></param>
		/// <param name="currentFrameSignalSemaphore: rendering finished, signal it"></param>
		void SubmitToQueue(VkCommandBuffer commandBuffer, const Fence& currentFrameFence, const Semaphore& currentFrameWaitSemaphore, const Semaphore& currentFrameSignalSemaphore);
		void Destroy();

		inline VkExtent2D GetExtent() { return m_SelectedSwapExtent; }
		inline VkSurfaceFormatKHR& GetSurfaceFormat() { return m_SelectedFormat; }
		inline VkSwapchainKHR GetVulkanHandle() { return m_SwapChain; }

		inline size_t GetImageCount() { return m_SwapChainImages.size(); }
		inline const std::vector<VulkanImageView>& GetImageViews() const { return m_SwapChainImageViews; }
	private:
		VkSwapchainKHR Create(VkSwapchainKHR oldSwapChain);

		SwapChainCapabilities GetSwapChainCapabilities(VkPhysicalDevice device);
		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const SwapChainCapabilities& capabilities);
		VkPresentModeKHR ChooseSwapPresentMode(const SwapChainCapabilities& capabilities);
		VkExtent2D ChooseSwapExtent(SwapChainCapabilities& capabilities);

		VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;
		VkSwapchainKHR m_OldSwapChain = VK_NULL_HANDLE;

		std::vector<VkImage> m_SwapChainImages;
		std::vector<VulkanImageView> m_SwapChainImageViews;

		VkSurfaceFormatKHR m_SelectedFormat;
		VkPresentModeKHR m_SelectedPresentMode;
		VkExtent2D m_SelectedSwapExtent;

		Ref<Window> m_Window = nullptr;
	};
}