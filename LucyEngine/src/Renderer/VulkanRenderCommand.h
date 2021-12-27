#pragma once

#include "RenderCommand.h"

namespace Lucy {

	class VulkanRenderCommand : public RenderCommand {
	public:
		VulkanRenderCommand();
		virtual ~VulkanRenderCommand() = default;

		void Begin(RefLucy<Pipeline> pipeline);
		void End(RefLucy<Pipeline> pipeline);
		void Draw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
		
		inline VkCommandBuffer GetCommandBuffer(uint32_t index) { return m_CommandBuffers[index]; }

		void Destroy();
	private:
		VkCommandPool m_CommandPool = VK_NULL_HANDLE;
		std::vector<VkCommandBuffer> m_CommandBuffers;
	};
}