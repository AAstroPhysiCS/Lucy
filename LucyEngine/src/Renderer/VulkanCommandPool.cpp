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
#include "Renderer/VulkanRenderer.h"

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
		return nullptr;
	}

	void VulkanCommandPool::Allocate() {
		Renderer::Submit([this]() {
			const VulkanDevice& device = VulkanDevice::Get();

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

		//TODO: Delete (all temporary, test stuff)
		auto uniformBuffer = pipeline->GetUniformBuffers<VulkanUniformBuffer>(0);

		struct MVP {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
		};

		MVP mvp;
		mvp.model = glm::rotate(glm::mat4(1.0f), 0.5f * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		mvp.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		mvp.proj = glm::perspective(glm::radians(45.0f), (float)extent.width / extent.height, 0.1f, 10.0f);
		mvp.proj[1][1] *= -1;

		uniformBuffer->SetData((void*)&mvp, sizeof(MVP), 0);
		uniformBuffer->WriteToSets(VulkanRenderer::CURRENT_FRAME);

		struct Add {
			glm::vec4 addition;
		};

		Add add;
		add.addition.x = 0.5f;
		add.addition.y = 0.5f;

		auto& uniformBuffer2 = pipeline->GetUniformBuffers<VulkanUniformBuffer>(1);
		uniformBuffer2->SetData((void*)&add, sizeof(Add), 0);
		uniformBuffer2->WriteToSets(VulkanRenderer::CURRENT_FRAME);

		Add add2;
		add2.addition.x = 0.5f;
		add2.addition.y = 0.5f;

		auto& uniformBuffer3 = pipeline->GetUniformBuffers<VulkanUniformBuffer>(2);
		uniformBuffer3->SetData((void*)&add2, sizeof(Add), 0);
		uniformBuffer3->WriteToSets(VulkanRenderer::CURRENT_FRAME);

		Add add3;
		add3.addition.x = 0.0f;
		add3.addition.y = 0.5f;

		auto& uniformBuffer4 = pipeline->GetUniformBuffers<VulkanUniformBuffer>(3);
		uniformBuffer4->SetData((void*)&add3, sizeof(Add), 0);
		uniformBuffer4->WriteToSets(VulkanRenderer::CURRENT_FRAME);

		std::vector<VkDescriptorSet> allSetsToBind = {
			uniformBuffer->GetDescriptorSet().GetSetBasedOffFrameIndex(VulkanRenderer::CURRENT_FRAME),
			uniformBuffer3->GetDescriptorSet().GetSetBasedOffFrameIndex(VulkanRenderer::CURRENT_FRAME)
		};

		for (uint32_t i = 0; i < m_CommandBuffers.size(); i++) {
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
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

				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipelineLayout(), 0, allSetsToBind.size(), allSetsToBind.data(), 0, nullptr);

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

	VkCommandBuffer VulkanCommandPool::BeginSingleTimeCommand() {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.commandBufferCount = 1;

		const VulkanDevice& vulkanDevice = VulkanDevice::Get();

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(vulkanDevice.GetLogicalDevice(), &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}

	void VulkanCommandPool::EndSingleTimeCommand(VkCommandBuffer commandBuffer) {
		const VulkanDevice& vulkanDevice = VulkanDevice::Get();
		
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
