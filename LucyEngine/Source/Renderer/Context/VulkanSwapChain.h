#pragma once

#include "SwapChain.h"

#include "vulkan/vulkan.h"
#include "Renderer/Image/VulkanImage2D.h"
#include "Renderer/Memory/Buffer/Vulkan/VulkanFrameBuffer.h"

#include "Renderer/Device/VulkanRenderDevice.h"

namespace Lucy {

	class Window;
	class RenderCommandQueue;

	class Semaphore;
	class Fence;

	struct SwapChainCapabilities {
		VkSurfaceCapabilitiesKHR surfaceCapabilities{};
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	class VulkanSwapChain final : public SwapChain {
	public:
		VulkanSwapChain(const Ref<Window>& window, const Ref<RenderDevice>& renderDevice);
		virtual ~VulkanSwapChain() = default;

		void Init() final override;
		void Recreate() final override;
		RenderContextResultCodes AcquireNextImage(const Semaphore& currentFrameImageAvailSemaphore, uint32_t& imageIndex) final override;
		RenderContextResultCodes Present(const Semaphore& signalSemaphore, uint32_t& imageIndex) final override;

		void Destroy() final override;

		inline const Unique<VulkanSwapChainFrameBuffer>& GetFrameBuffer() const { return m_SwapChainFrameBuffer; }
		inline Ref<VulkanRenderPass> GetRenderPass() const { return m_SwapChainRenderPass; }

		inline VkExtent2D GetExtent() const { return m_SelectedSwapExtent; }
		inline const VkSurfaceFormatKHR& GetSurfaceFormat() const { return m_SelectedFormat; }
		inline size_t GetSwapChainImageCount() const { return m_SwapChainImages.size(); }
	private:
		VkSwapchainKHR Create(VkSwapchainKHR oldSwapChain);

		SwapChainCapabilities GetSwapChainCapabilities(VkPhysicalDevice device) const;
		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const SwapChainCapabilities& capabilities) const;
		VkPresentModeKHR ChooseSwapPresentMode(const SwapChainCapabilities& capabilities) const;
		VkExtent2D ChooseSwapExtent(const SwapChainCapabilities& capabilities) const;

		VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;
		VkSwapchainKHR m_OldSwapChain = VK_NULL_HANDLE;

		std::vector<VkImage> m_SwapChainImages;
		std::vector<VulkanImageView> m_SwapChainImageViews;
		Unique<VulkanSwapChainFrameBuffer> m_SwapChainFrameBuffer = nullptr;
		Ref<VulkanRenderPass> m_SwapChainRenderPass = nullptr;

		VkSurfaceFormatKHR m_SelectedFormat;
		VkPresentModeKHR m_SelectedPresentMode;
		VkExtent2D m_SelectedSwapExtent;

		friend class RendererBackend; //for swapchain image count

		friend VkSwapchainCreateInfoKHR VulkanAPI::SwapchainCreateInfo(const void* swapChainInstance, uint32_t imageCount, VkSwapchainKHR oldSwapChain, VkSurfaceTransformFlagBitsKHR preTransform);
	};
}