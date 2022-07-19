#pragma once

#include "../FrameBuffer.h"
#include "Renderer/VulkanRenderPass.h"

namespace Lucy {

	class VulkanFrameBuffer : public FrameBuffer {
	public:
		VulkanFrameBuffer(FrameBufferCreateInfo& createInfo);
		virtual ~VulkanFrameBuffer() = default;
		
		void Destroy() override;

		inline std::vector<VkFramebuffer>& GetVulkanHandles() { return m_FrameBufferHandles; }
		inline std::vector<Ref<VulkanImage2D>>& GetImages() { return m_Images; }
	private:
		void Recreate(uint32_t width, uint32_t height, Ref<void> internalInfo = nullptr);
		void Create();
		void CreateDepthImage();

		std::vector<VkFramebuffer> m_FrameBufferHandles;
		Ref<VulkanRenderPass> m_RenderPass;

		std::vector<Ref<VulkanImage2D>> m_Images;
		std::vector<VulkanImageView> m_ImageViews;

		Ref<VulkanImage2D> m_DepthImage = nullptr;

		friend class VulkanRHI; //for OnViewportResize (ImGui)
		friend class VulkanPipeline; //for Pipeline Recreate
	};
}

