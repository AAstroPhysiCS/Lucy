#pragma once

#include "vulkan/vulkan.h"

#include "Renderer/Synchronization/SynchItems.h"

#include "Renderer/Image/VulkanImage.h"
#include "Renderer/Buffer/FrameBuffer.h"

namespace Lucy {

	class CommandQueue;

	struct SwapChainCapabilities {
		VkSurfaceCapabilitiesKHR surfaceCapabilities{};
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	class VulkanSwapChain {
	private:
		const uint32_t MAX_FRAMES_IN_FLIGHT = 3;

		uint32_t m_ImageIndex = 0;
		uint32_t m_CurrentFrameIndex = 0;

		VulkanSwapChain();
	public:
		~VulkanSwapChain() = default;
		static VulkanSwapChain& Get();

		void Create();
		void Recreate();
		void BeginFrame();
		void EndFrame(const CommandQueue& commandQueue);
		VkResult Present();
		void Destroy();

		inline VkExtent2D& GetExtent() { return m_SelectedSwapExtent; }
		inline VkSurfaceFormatKHR& GetSurfaceFormat() { return m_SelectedFormat; }
		inline VkSwapchainKHR GetVulkanHandle() { return m_SwapChain; }
		inline VkResult GetLastSwapChainResult() { return m_LastSwapChainResult; }

		inline uint32_t GetImageCount() { return m_SwapChainImages.size(); }
		inline std::vector<VulkanImageView>& GetImageViews() { return m_SwapChainImageViews; }
		
		inline uint32_t GetCurrentImageIndex() { return m_ImageIndex; }
		inline uint32_t GetCurrentFrameIndex() { return m_CurrentFrameIndex; }
		inline const uint32_t GetMaxFramesInFlight() const { return MAX_FRAMES_IN_FLIGHT; }
		inline RefLucy<VulkanRHIFrameBufferDesc> GetSwapChainFrameBufferDesc() const { return m_SwapChainFrameBufferDesc; }
	private:
		VkSwapchainKHR Create(VkSwapchainKHR oldSwapChain);
		VkResult AcquireNextImage(VkSemaphore currentFrameImageAvailSemaphore, uint32_t& imageIndex);

		SwapChainCapabilities GetSwapChainCapabilities(VkPhysicalDevice device);
		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const SwapChainCapabilities& capabilities);
		VkPresentModeKHR ChooseSwapPresentMode(const SwapChainCapabilities& capabilities);
		VkExtent2D ChooseSwapExtent(SwapChainCapabilities& capabilities);

		VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;
		VkSwapchainKHR m_OldSwapChain = VK_NULL_HANDLE;

		std::vector<VkImage> m_SwapChainImages;
		std::vector<VulkanImageView> m_SwapChainImageViews;
		RefLucy<VulkanRHIFrameBufferDesc> m_SwapChainFrameBufferDesc = CreateRef<VulkanRHIFrameBufferDesc>();

		VkSurfaceFormatKHR m_SelectedFormat;
		VkPresentModeKHR m_SelectedPresentMode;
		VkExtent2D m_SelectedSwapExtent;

		std::vector<Semaphore> m_WaitSemaphores;
		std::vector<Semaphore> m_SignalSemaphores;
		std::vector<Fence> m_InFlightFences;

		VkResult m_LastSwapChainResult;
	};
}