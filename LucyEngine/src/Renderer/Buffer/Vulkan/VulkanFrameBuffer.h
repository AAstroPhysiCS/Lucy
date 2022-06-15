#pragma once

#include "../FrameBuffer.h"
#include "Renderer/Context/VulkanSwapChain.h"
#include "Renderer/VulkanRenderPass.h"

namespace Lucy {

	class VulkanFrameBuffer : public FrameBuffer {
	public:
		VulkanFrameBuffer(FrameBufferSpecification& specs);
		virtual ~VulkanFrameBuffer() = default;
		
		void Destroy() override;

		inline std::vector<VkFramebuffer>& GetVulkanHandles() { return m_FrameBufferHandles; }
		inline std::vector<RefLucy<VulkanImage2D>>& GetImages() { return m_Images; }
	private:
		void Recreate(uint32_t width, uint32_t height, RefLucy<void> internalInfo = nullptr);
		void Create();
		void CreateImagesBasedOnDescs();
		void CreateDepthImage();

		std::vector<VkFramebuffer> m_FrameBufferHandles;
		RefLucy<VulkanRenderPass> m_RenderPass;

		std::vector<RefLucy<VulkanImage2D>> m_Images;
		std::vector<VulkanImageView> m_ImageViews;

		RefLucy<VulkanImage2D> m_DepthImage = nullptr;

		friend class VulkanRHI; //for OnViewportResize (ImGui)
		friend class VulkanPipeline; //for Pipeline Recreate
	};
}

