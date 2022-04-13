#include "lypch.h"
#include "VulkanCommandPool.h"

#include "Renderer/Context/VulkanDevice.h"
#include "Renderer.h"

namespace Lucy {

	VulkanCommandPool::VulkanCommandPool(CommandPoolSpecs specs)
		: m_Specs(specs) {
		Allocate();
	}

	RefLucy<VulkanCommandPool> VulkanCommandPool::Create(CommandPoolSpecs specs) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::Vulkan:
				return CreateRef<VulkanCommandPool>(specs);
				break;
		}
		LUCY_ASSERT(false);
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
	}

	void VulkanCommandPool::Destroy() {
		vkDestroyCommandPool(VulkanDevice::Get().GetLogicalDevice(), m_CommandPool, nullptr);
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
}
