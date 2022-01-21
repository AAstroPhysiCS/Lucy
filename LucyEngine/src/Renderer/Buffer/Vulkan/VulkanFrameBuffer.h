#pragma once

#include "../FrameBuffer.h"
#include "Renderer/Context/VulkanSwapChain.h"
#include "Renderer/VulkanRenderPass.h"

namespace Lucy {

	class VulkanFrameBuffer : public FrameBuffer {

	public:
		VulkanFrameBuffer(FrameBufferSpecification& specs, RefLucy<VulkanRenderPass>& renderPass);
		virtual ~VulkanFrameBuffer() = default;

		void Bind();
		void Unbind();
		void Destroy();
		void Blit();
		void Resize(int32_t width, int32_t height);

		inline std::vector<VkFramebuffer>& GetSwapChainFrameBuffers() { return m_SwapChainFrameBuffers; }
	private:
		void Create();

		std::vector<VkFramebuffer> m_SwapChainFrameBuffers;
		RefLucy<VulkanRenderPass> m_RenderPass;
	};
}

