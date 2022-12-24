#include "lypch.h"
#include "VulkanCommandPool.h"

#include "Renderer/Context/VulkanContextDevice.h"
#include "Renderer/Renderer.h"

namespace Lucy {

	VulkanCommandPool::VulkanCommandPool(const CommandPoolCreateInfo& createInfo)
		: CommandPool(createInfo) {
		Renderer::EnqueueToRenderThread([this]() {
			Allocate();
		});
	}

	void VulkanCommandPool::Allocate() {
		const VulkanContextDevice& device = VulkanContextDevice::Get();

		VkCommandPoolCreateInfo createCommandPoolInfo = VulkanAPI::CommandPoolCreateInfo(m_CreateInfo.PoolFlags, device.GetQueueFamilies().GraphicsFamily);
		LUCY_VK_ASSERT(vkCreateCommandPool(device.GetLogicalDevice(), &createCommandPoolInfo, nullptr, &m_CommandPool));

		m_CommandBuffers.resize(m_CreateInfo.CommandBufferCount);

		VkCommandBufferAllocateInfo createAllocInfo = VulkanAPI::CommandBufferAllocateInfo(m_CommandPool, m_CreateInfo.Level, m_CreateInfo.CommandBufferCount);
		LUCY_VK_ASSERT(vkAllocateCommandBuffers(device.GetLogicalDevice(), &createAllocInfo, m_CommandBuffers.data()));
	}

	void VulkanCommandPool::Destroy() {
		vkDestroyCommandPool(VulkanContextDevice::Get().GetLogicalDevice(), m_CommandPool, nullptr);
	}

	void VulkanCommandPool::Recreate() {
		vkFreeCommandBuffers(VulkanContextDevice::Get().GetLogicalDevice(), m_CommandPool, m_CreateInfo.CommandBufferCount, m_CommandBuffers.data());

		VkCommandBufferAllocateInfo createAllocInfo = VulkanAPI::CommandBufferAllocateInfo(m_CommandPool, m_CreateInfo.Level, m_CreateInfo.CommandBufferCount);
		LUCY_VK_ASSERT(vkAllocateCommandBuffers(VulkanContextDevice::Get().GetLogicalDevice(), &createAllocInfo, m_CommandBuffers.data()));
	}

	VkCommandBuffer VulkanCommandPool::BeginSingleTimeCommand() {
		VkCommandBufferAllocateInfo allocInfo = VulkanAPI::CommandBufferAllocateInfo(m_CommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

		const VulkanContextDevice& vulkanDevice = VulkanContextDevice::Get();

		VkCommandBuffer immediateCommandBuffer;
		vkAllocateCommandBuffers(vulkanDevice.GetLogicalDevice(), &allocInfo, &immediateCommandBuffer);
		m_CommandBuffers.push_back(immediateCommandBuffer);

		VkCommandBufferBeginInfo beginInfo = VulkanAPI::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		vkBeginCommandBuffer(immediateCommandBuffer, &beginInfo);

		return immediateCommandBuffer;
	}

	void VulkanCommandPool::EndSingleTimeCommand() {
		vkEndCommandBuffer(m_CommandBuffers[m_CommandBuffers.size() - 1]);
	}

	void VulkanCommandPool::FreeCommandBuffers(uint32_t commandBufferCount, size_t commandBufferStartIndex) {
		vkFreeCommandBuffers(VulkanContextDevice::Get().GetLogicalDevice(), m_CommandPool, commandBufferCount, &m_CommandBuffers[commandBufferStartIndex]);
		m_CommandBuffers.erase(m_CommandBuffers.begin() + commandBufferStartIndex, m_CommandBuffers.begin() + commandBufferStartIndex + commandBufferCount);
	}
}
