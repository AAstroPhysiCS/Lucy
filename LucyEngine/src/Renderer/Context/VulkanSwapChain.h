#pragma once

#include "vulkan/vulkan.h"

#include "Renderer/VulkanCommandPool.h"
#include "Renderer/Synchronization/SynchItems.h"

#include "Renderer/Texture/VulkanImage.h"

namespace Lucy {

	class RenderCommandQueue;

	struct SwapChainCapabilities {
		VkSurfaceCapabilitiesKHR surfaceCapabilities{};
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	class VulkanSwapChain {
	public:
		inline static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
	private:
		inline static uint32_t s_ImageIndex = 0;
		inline static uint32_t s_CurrentFrameIndex = 0;

		VulkanSwapChain() = default;
	public:
		~VulkanSwapChain() = default;
		static VulkanSwapChain& Get();

		void Create();
		void Recreate();
		void BeginFrame();
		void Execute(const RenderCommandQueue& renderCommand);
		void EndFrame();
		VkResult Present();
		void Destroy();

		VkCommandBuffer BeginSingleTimeCommand();
		void EndSingleTimeCommand(VkCommandBuffer commandBuffer);

		inline VkExtent2D& GetExtent() { return m_SelectedSwapExtent; }
		inline VkSurfaceFormatKHR& GetSurfaceFormat() { return m_SelectedFormat; }
		inline VkSwapchainKHR GetVulkanHandle() { return m_SwapChain; }

		inline uint32_t GetImageCount() { return m_SwapChainImages.size(); }
		inline std::vector<VulkanImageView>& GetImageViews() { return m_SwapChainImageViews; }

		inline RefLucy<VulkanCommandPool> GetCommandPool() { return m_CommandPool; }

		inline static uint32_t GetCurrentImageIndex() { return s_ImageIndex; }
		inline static uint32_t GetCurrentFrameIndex() { return s_CurrentFrameIndex; }
	private:
		VkSwapchainKHR Create(VkSwapchainKHR oldSwapChain);
		void AfterInitialization();
		VkResult AcquireNextImage(VkSemaphore currentFrameImageAvailSemaphore, uint32_t& imageIndex);

		SwapChainCapabilities GetSwapChainCapabilities(VkPhysicalDevice device);
		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const SwapChainCapabilities& capabilities);
		VkPresentModeKHR ChooseSwapPresentMode(const SwapChainCapabilities& capabilities);
		VkExtent2D ChooseSwapExtent(SwapChainCapabilities& capabilities);

		VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;
		VkSwapchainKHR m_OldSwapChain = VK_NULL_HANDLE;

		std::vector<VkImage> m_SwapChainImages;
		std::vector<VulkanImageView> m_SwapChainImageViews;
		RefLucy<VulkanCommandPool> m_CommandPool;

		VkSurfaceFormatKHR m_SelectedFormat;
		VkPresentModeKHR m_SelectedPresentMode;
		VkExtent2D m_SelectedSwapExtent;

		std::vector<Semaphore> m_ImageIsAvailableSemaphores;
		std::vector<Semaphore> m_RenderIsFinishedSemaphores;
		std::vector<Fence> m_InFlightFences;

		bool m_FirstInitialized = false;
		VkResult m_LastSwapChainResult;

		friend class VulkanRenderer;
	};
}