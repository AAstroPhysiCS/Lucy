#include "lypch.h"
#include "VulkanRenderCommand.h"

#include "Context/VulkanPipeline.h"
#include "Renderer/Buffer/Vulkan/VulkanFrameBuffer.h"

namespace Lucy {
	
	VulkanRenderCommand::VulkanRenderCommand() {
		VulkanDevice& device = VulkanDevice::Get();

		VkCommandPoolCreateInfo createCommandPoolInfo{};
		createCommandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		createCommandPoolInfo.flags = 0;
		createCommandPoolInfo.queueFamilyIndex = device.GetQueueFamilies().GraphicsFamily;
		LUCY_VULKAN_ASSERT(vkCreateCommandPool(device.GetLogicalDevice(), &createCommandPoolInfo, nullptr, &m_CommandPool));

		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		m_CommandBuffers.resize(swapChain.m_SwapChainImageViews.size());

		VkCommandBufferAllocateInfo createAllocInfo{};
		createAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		createAllocInfo.commandPool = m_CommandPool;
		createAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		createAllocInfo.commandBufferCount = m_CommandBuffers.size();

		LUCY_VULKAN_ASSERT(vkAllocateCommandBuffers(device.GetLogicalDevice(), &createAllocInfo, m_CommandBuffers.data()));
	}

	void VulkanRenderCommand::Begin(RefLucy<Pipeline> pipeline) {
		//TODO: Change
		static bool firstFrame = false;
		
		if (firstFrame) return;
		firstFrame = true;

		auto& renderPass = pipeline->GetRenderPass();
		auto& vulkanPipeline = As(pipeline, VulkanPipeline);
		auto& framebuffer = As(pipeline->GetFrameBuffer(), VulkanFrameBuffer)->GetSwapChainFrameBuffers();
		auto& extent = VulkanSwapChain::Get().GetExtent();
		
		for (uint32_t i = 0; i < m_CommandBuffers.size(); i++) {
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.pInheritanceInfo = nullptr;

			LUCY_VULKAN_ASSERT(vkBeginCommandBuffer(m_CommandBuffers[i], &beginInfo));
			
			RenderPassBeginInfo info;
			info.CommandBuffer = m_CommandBuffers[i];
			info.FrameBuffer = framebuffer[i];
			renderPass->Begin(info);

			VkViewport viewport;
			viewport.x = 0;
			viewport.y = 0;
			viewport.width = extent.width;
			viewport.height = extent.height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			VkRect2D scissor{};
			scissor.offset = { 0, 0 };
			scissor.extent = extent;
			
			vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->GetVulkanHandle());
			vkCmdSetViewport(m_CommandBuffers[i], 0, 1, &viewport);
			vkCmdSetScissor(m_CommandBuffers[i], 0, 1, &scissor);

			Draw(m_CommandBuffers[i], 3, 1, 0, 0);
		}
	}
	
	void VulkanRenderCommand::End(RefLucy<Pipeline> pipeline) {
		//TODO: Change
		static bool firstFrame = false;

		if (firstFrame) return;
		firstFrame = true;

		auto& renderPass = pipeline->GetRenderPass();

		for (uint32_t i = 0; i < m_CommandBuffers.size(); i++) {
			RenderPassEndInfo info;
			info.CommandBuffer = m_CommandBuffers[i];
			renderPass->End(info);

			LUCY_VULKAN_ASSERT(vkEndCommandBuffer(m_CommandBuffers[i]));
		}
	}

	void VulkanRenderCommand::Draw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
		vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void VulkanRenderCommand::Destroy() {
		vkDestroyCommandPool(VulkanDevice::Get().GetLogicalDevice(), m_CommandPool, nullptr);
	}
}