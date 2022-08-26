#pragma once

#include "../FrameBuffer.h"

#include "Renderer/VulkanRenderPass.h"
#include "Renderer/Image/VulkanImage.h"

namespace Lucy {

	class VulkanFrameBuffer : public FrameBuffer {
	public:
		VulkanFrameBuffer(FrameBufferCreateInfo& createInfo);
		virtual ~VulkanFrameBuffer() = default;
		
		void Destroy() final override;

		inline std::vector<VkFramebuffer>& GetVulkanHandles() { return m_FrameBufferHandles; }
		inline std::vector<Ref<VulkanImage2D>>& GetImages() { return m_Images; }

		void Recreate(uint32_t width, uint32_t height) final override;
		void Recreate(uint32_t width, uint32_t height, const std::vector<VulkanImageView>& swapChainImageViews = { });
	private:
		void Create();
		void CreateDepthImage();

		std::vector<VkFramebuffer> m_FrameBufferHandles;

		std::vector<Ref<VulkanImage2D>> m_Images;
		Ref<VulkanRenderPass> m_RenderPass = nullptr;

		Ref<VulkanImage2D> m_DepthImage = nullptr;
	};
}

