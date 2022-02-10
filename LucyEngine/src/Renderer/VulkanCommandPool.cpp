#include "lypch.h"
#include "VulkanCommandPool.h"

#include "Renderer/Context/VulkanDevice.h"
#include "Renderer/RenderPass.h"

#include "Context/VulkanSwapChain.h"
#include "Context/VulkanPipeline.h"

#include "Buffer/Vulkan/VulkanFrameBuffer.h"
#include "Buffer/Vulkan/VulkanVertexBuffer.h" //TODO: Delete
#include "Buffer/Vulkan/VulkanIndexBuffer.h" //TODO: Delete
#include "Renderer.h" //TODO: Delete

namespace Lucy {

	VulkanVertexBuffer* vertexBuffer = nullptr; //TODO: Delete
	VulkanIndexBuffer* indexBuffer = nullptr; //TODO: Delete

	VulkanCommandPool::VulkanCommandPool(CommandPoolSpecs specs)
		: m_Specs(specs) {
		Allocate();
	}

	RefLucy<VulkanCommandPool> VulkanCommandPool::Create(CommandPoolSpecs specs) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::Vulkan:
				return CreateRef<VulkanCommandPool>(specs);
				break;
			case RenderArchitecture::OpenGL:
				LUCY_ASSERT(false);
				break;
		}
	}

	void VulkanCommandPool::Allocate() {
		Renderer::Submit([this]() {
			VulkanDevice& device = VulkanDevice::Get();

			VkCommandPoolCreateInfo createCommandPoolInfo{};
			createCommandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			createCommandPoolInfo.flags = m_Specs.PoolFlags;
			createCommandPoolInfo.queueFamilyIndex = device.GetQueueFamilies().GraphicsFamily;
			LUCY_VK_ASSERT(vkCreateCommandPool(device.GetLogicalDevice(), &createCommandPoolInfo, nullptr, &m_CommandPool));

			m_CommandBuffers.resize(m_Specs.CommandBufferCount);

			VkCommandBufferAllocateInfo createAllocInfo{};
			createAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			createAllocInfo.commandPool = m_CommandPool;
			createAllocInfo.level = m_Specs.Level;
			createAllocInfo.commandBufferCount = m_Specs.CommandBufferCount;

			LUCY_VK_ASSERT(vkAllocateCommandBuffers(device.GetLogicalDevice(), &createAllocInfo, m_CommandBuffers.data()));
		});

		vertexBuffer = new VulkanVertexBuffer(8);
		indexBuffer = new VulkanIndexBuffer(6);
		vertexBuffer->AddData(
			{ -0.5f, -0.5f,
			   0.5f, -0.5f,
			   0.5f, 0.5f,
			  -0.5f, 0.5f
			}
		);
		vertexBuffer->Load();

		indexBuffer->AddData(
			{ 0, 1, 2, 2, 3, 0 }
		);
		indexBuffer->Load();
	}

	void VulkanCommandPool::Destroy() {
		vkDestroyCommandPool(VulkanDevice::Get().GetLogicalDevice(), m_CommandPool, nullptr);

		vertexBuffer->Destroy(); //TODO: Delete
		indexBuffer->Destroy(); //TODO: Delete

		delete vertexBuffer;
		delete indexBuffer;
	}

	void VulkanCommandPool::BeginRecording(RefLucy<VulkanPipeline>& pipeline) {
		auto& renderPass = pipeline->GetRenderPass();
		auto& framebuffer = As(pipeline->GetFrameBuffer(), VulkanFrameBuffer)->GetSwapChainFrameBuffers();
		auto& extent = VulkanSwapChain::Get().GetExtent();

		for (uint32_t i = 0; i < m_CommandBuffers.size(); i++) {
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			beginInfo.pInheritanceInfo = nullptr;
			auto commandBuffer = m_CommandBuffers[i];
			LUCY_VK_ASSERT(vkBeginCommandBuffer(commandBuffer, &beginInfo));

			RenderPassBeginInfo info;
			info.CommandBuffer = commandBuffer;
			info.VulkanFrameBuffer = framebuffer[i];
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

			if (viewport.width != 0 || viewport.height != 0) {
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetVulkanHandle());
				vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
				vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

				vertexBuffer->Bind({ commandBuffer });
				indexBuffer->Bind({ commandBuffer });
				vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);
				vertexBuffer->Unbind();
				indexBuffer->Unbind();
			}
		}
	}

	void VulkanCommandPool::EndRecording(RefLucy<VulkanPipeline>& pipeline) {
		auto& renderPass = pipeline->GetRenderPass();
		for (uint32_t i = 0; i < m_CommandBuffers.size(); i++) {
			auto commandBuffer = m_CommandBuffers[i];
			RenderPassEndInfo info;
			info.CommandBuffer = commandBuffer;
			renderPass->End(info);
			LUCY_VK_ASSERT(vkEndCommandBuffer(commandBuffer));
		}
	}

	// Should not be used in a loop
	void VulkanCommandPool::DirectCopyBuffer(VkBuffer& stagingBuffer, VkBuffer& buffer, VkDeviceSize size) {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.commandBufferCount = 1;

		VulkanDevice& vulkanDevice = VulkanDevice::Get();

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(vulkanDevice.GetLogicalDevice(), &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer, buffer, 1, &copyRegion);

		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		const VkQueue& graphicsQueue = vulkanDevice.GetGraphicsQueue();
		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);

		vkFreeCommandBuffers(vulkanDevice.GetLogicalDevice(), m_CommandPool, 1, &commandBuffer);
	}

	void VulkanCommandPool::Execute(RefLucy<VulkanPipeline>& pipeline) {
		BeginRecording(pipeline);
		EndRecording(pipeline);
	}

	void VulkanCommandPool::Recreate() {
		vkFreeCommandBuffers(VulkanDevice::Get().GetLogicalDevice(), m_CommandPool, m_Specs.CommandBufferCount, m_CommandBuffers.data());

		VkCommandBufferAllocateInfo createAllocInfo{};
		createAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		createAllocInfo.commandPool = m_CommandPool;
		createAllocInfo.level = m_Specs.Level;
		createAllocInfo.commandBufferCount = m_Specs.CommandBufferCount;

		LUCY_VK_ASSERT(vkAllocateCommandBuffers(VulkanDevice::Get().GetLogicalDevice(), &createAllocInfo, m_CommandBuffers.data()));
	}
}
