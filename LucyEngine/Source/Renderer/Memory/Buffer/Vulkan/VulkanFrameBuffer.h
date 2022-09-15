#pragma once

#include <unordered_map>
#include "../FrameBuffer.h"

#include "Renderer/VulkanRenderPass.h"
#include "Renderer/Image/VulkanImage2D.h"

namespace Lucy {

	class VulkanFrameBuffer : public FrameBuffer {
	public:
		VulkanFrameBuffer(FrameBufferCreateInfo& createInfo);
		virtual ~VulkanFrameBuffer() = default;
		
		void Destroy() final override;

		inline const std::vector<VkFramebuffer>& GetVulkanHandles() const { return m_FrameBufferHandles; }
		inline const std::vector<Ref<VulkanImage2D>>& GetImages() const { return m_Images; }
		inline bool IsInFlight() const { return m_CreateInfo.IsInFlight; }

		void Recreate(uint32_t width, uint32_t height) final override;
		void Recreate(uint32_t width, uint32_t height, const std::vector<VulkanImageView>& swapChainImageViews = { });
	private:
		void Create();
		//should only be used for swapchain imageviews
		void CreateForSwapChain();

		std::vector<VkFramebuffer> m_FrameBufferHandles;
		std::vector<Ref<VulkanImage2D>> m_Images;
		Ref<VulkanImage2D> m_DepthImage = nullptr;

		bool m_CreatedInFlightFrameBufferImages = false;

		Ref<VulkanRenderPass> m_RenderPass = nullptr;
	};
}

