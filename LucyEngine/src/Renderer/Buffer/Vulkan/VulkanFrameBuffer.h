#pragma once

#include "../FrameBuffer.h"
#include "Renderer/Context/VulkanSwapChain.h"
#include "Renderer/VulkanRenderPass.h"

namespace Lucy {

	class VulkanFrameBuffer : public FrameBuffer {

	public:
		VulkanFrameBuffer(FrameBufferSpecification& specs);
		virtual ~VulkanFrameBuffer() = default;

		void Bind() override;
		void Unbind() override;
		void Destroy() override;
		void Recreate();

		inline std::vector<VkFramebuffer>& GetVulkanHandles() { return m_FrameBufferHandles; }
		inline std::vector<RefLucy<VulkanImage2D>>& GetImages() { return m_Images; }

		inline uint32_t GetWidth() { return m_Specs.Width; }
		inline uint32_t GetHeight() { return m_Specs.Height; }
	private:
		void Create();

		std::vector<VkFramebuffer> m_FrameBufferHandles;
		RefLucy<VulkanRenderPass> m_RenderPass;

		std::vector<RefLucy<VulkanImage2D>> m_Images;
		std::vector<VulkanImageView> m_ImageViews;
	};
}

