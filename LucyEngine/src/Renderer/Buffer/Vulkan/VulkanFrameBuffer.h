#pragma once

#include "../FrameBuffer.h"
#include "Renderer/Context/VulkanSwapChain.h"
#include "Renderer/VulkanRenderPass.h"

namespace Lucy {

	class VulkanFrameBuffer : public FrameBuffer {

	public:
		VulkanFrameBuffer(FrameBufferSpecification& specs, RefLucy<VulkanRenderPass>& renderPass);
		virtual ~VulkanFrameBuffer() = default;

		void Bind() override;
		void Unbind() override;
		void Destroy() override;
		void Recreate();

		inline std::vector<VkFramebuffer>& GetVulkanHandles() { return m_FrameBufferHandles; }
	private:
		void Create();

		std::vector<VkFramebuffer> m_FrameBufferHandles;
		RefLucy<VulkanRenderPass> m_RenderPass;
	};
}

