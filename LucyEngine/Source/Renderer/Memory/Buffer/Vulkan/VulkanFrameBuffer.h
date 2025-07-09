#pragma once

#include <unordered_map>
#include "../FrameBuffer.h"

#include "Renderer/VulkanRenderPass.h"
#include "Renderer/Image/VulkanImage2D.h"

#include "Renderer/Device/VulkanRenderDevice.h"

namespace Lucy {

	class VulkanFrameBuffer : public FrameBuffer {
	public:
		VulkanFrameBuffer(const FrameBufferCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device);
		virtual ~VulkanFrameBuffer() = default;
		
		inline const std::vector<VkFramebuffer>& GetVulkanHandles() const { return m_FrameBufferHandles; }
		inline const std::vector<RenderResourceHandle>& GetImageHandles() const { return m_ImageHandles; }
		inline bool IsInFlight() const { return m_CreateInfo.IsInFlight; }

		void RTRecreate(uint32_t width, uint32_t height) final override;
	private:
		void RTCreate();
		void RTDestroyResource() final override;
		void DestroyHandles();

		//Helper functions
		Ref<VulkanImage> GetImage(uint32_t index);
		Ref<VulkanImage> GetDepthImage();
		Ref<VulkanRenderPass> GetRenderPass();

		std::vector<VkFramebuffer> m_FrameBufferHandles;
		std::vector<RenderResourceHandle> m_ImageHandles;
		RenderResourceHandle m_DepthImageHandle = InvalidRenderResourceHandle;

		bool m_CreatedInFlightFrameBufferImages = false;

		Ref<VulkanRenderDevice> m_VulkanDevice = nullptr;
	};

	class VulkanSwapChainFrameBuffer : private FrameBuffer {
	public:
		VulkanSwapChainFrameBuffer(const Ref<VulkanRenderDevice>& vulkanDevice, const VkExtent2D& extent, const std::vector<VulkanImageView>& swapChainImageViews, const Ref<VulkanRenderPass>& renderPass);
		virtual ~VulkanSwapChainFrameBuffer() = default;

		inline const std::vector<VkFramebuffer>& GetVulkanHandles() const { return m_FrameBufferHandles; }

		inline uint32_t GetWidth() const { return m_CreateInfo.Width; }
		inline uint32_t GetHeight() const { return m_CreateInfo.Height; }

		void RTDestroyResource() final override;
	private:
		void CreateForSwapChain();

		void RTRecreate(uint32_t width, uint32_t height) final override;

		inline Ref<VulkanRenderPass> GetRenderPass() { return m_RenderPass; }

		const std::vector<VulkanImageView>& m_SwapChainImageViews;
		const Ref<VulkanRenderPass>& m_RenderPass = nullptr;

		std::vector<VkFramebuffer> m_FrameBufferHandles;
		Ref<VulkanRenderDevice> m_VulkanDevice = nullptr;

		friend class VulkanSwapChain;
	};
}

