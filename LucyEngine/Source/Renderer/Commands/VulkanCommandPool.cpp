#include "lypch.h"
#include "VulkanCommandPool.h"

#include "Renderer/Renderer.h"
#include "Renderer/Device/VulkanRenderDevice.h"

namespace Lucy {

	VulkanCommandPool::VulkanCommandPool(const CommandPoolCreateInfo& createInfo)
		: CommandPool(createInfo) {
		if (createInfo.CommandBufferCount == 0 && createInfo.Level == 0 && createInfo.PoolFlags == 0) //for transient, since transient also calls this constructor
			return;

		const auto& vulkanDevice = createInfo.RenderDevice->As<VulkanRenderDevice>();

		VkCommandPoolCreateInfo createCommandPoolInfo = VulkanAPI::CommandPoolCreateInfo(m_CreateInfo.PoolFlags, 
			m_CreateInfo.TargetQueueFamily == TargetQueueFamily::Graphics ? vulkanDevice->GetQueueFamilies().GraphicsFamily : vulkanDevice->GetQueueFamilies().ComputeFamily);
		LUCY_VK_ASSERT(vkCreateCommandPool(vulkanDevice->GetLogicalDevice(), &createCommandPoolInfo, nullptr, &m_CommandPool));

		m_CommandBuffers.resize(m_CreateInfo.CommandBufferCount);

		VkCommandBufferAllocateInfo createAllocInfo = VulkanAPI::CommandBufferAllocateInfo(m_CommandPool, m_CreateInfo.Level, m_CreateInfo.CommandBufferCount);
		LUCY_VK_ASSERT(vkAllocateCommandBuffers(vulkanDevice->GetLogicalDevice(), &createAllocInfo, m_CommandBuffers.data()));
	}

	void VulkanCommandPool::Destroy() {
		const auto& vulkanDevice = m_CreateInfo.RenderDevice->As<VulkanRenderDevice>();
		vkDestroyCommandPool(vulkanDevice->GetLogicalDevice(), m_CommandPool, nullptr);
	}

	void VulkanCommandPool::Recreate() {
		const auto& vulkanDevice = m_CreateInfo.RenderDevice->As<VulkanRenderDevice>();

		vkFreeCommandBuffers(vulkanDevice->GetLogicalDevice(), m_CommandPool, m_CreateInfo.CommandBufferCount, m_CommandBuffers.data());

		VkCommandBufferAllocateInfo createAllocInfo = VulkanAPI::CommandBufferAllocateInfo(m_CommandPool, m_CreateInfo.Level, m_CreateInfo.CommandBufferCount);
		LUCY_VK_ASSERT(vkAllocateCommandBuffers(vulkanDevice->GetLogicalDevice(), &createAllocInfo, m_CommandBuffers.data()));
	}

	void VulkanCommandPool::FreeCommandBuffers(uint32_t commandBufferCount, size_t commandBufferStartIndex) {
		const auto& vulkanDevice = m_CreateInfo.RenderDevice->As<VulkanRenderDevice>();

		vkFreeCommandBuffers(vulkanDevice->GetLogicalDevice(), m_CommandPool, commandBufferCount, &m_CommandBuffers[commandBufferStartIndex]);
		m_CommandBuffers.erase(m_CommandBuffers.begin() + commandBufferStartIndex, m_CommandBuffers.begin() + commandBufferStartIndex + commandBufferCount);
	}

	VulkanTransientCommandPool::VulkanTransientCommandPool(const Ref<VulkanRenderDevice>& vulkanDevice)
		: VulkanCommandPool(CommandPoolCreateInfo{ .CommandBufferCount = 0, .Level = 0, .PoolFlags = 0, .RenderDevice = vulkanDevice}) {
		VkCommandPoolCreateInfo createCommandPoolInfo = VulkanAPI::CommandPoolCreateInfo(m_CreateInfo.PoolFlags, 
			m_CreateInfo.TargetQueueFamily == TargetQueueFamily::Graphics ? vulkanDevice->GetQueueFamilies().GraphicsFamily : vulkanDevice->GetQueueFamilies().ComputeFamily);
		LUCY_VK_ASSERT(vkCreateCommandPool(vulkanDevice->GetLogicalDevice(), &createCommandPoolInfo, nullptr, &m_CommandPool));
	}

	VkCommandBuffer VulkanTransientCommandPool::BeginSingleTimeCommand(VkDevice logicalDevice) {
		VkCommandBufferAllocateInfo allocInfo = VulkanAPI::CommandBufferAllocateInfo(m_CommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

		VkCommandBuffer immediateCommandBuffer;
		vkAllocateCommandBuffers(logicalDevice, &allocInfo, &immediateCommandBuffer);
		m_CommandBuffers.push_back(immediateCommandBuffer);

		VkCommandBufferBeginInfo beginInfo = VulkanAPI::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		vkBeginCommandBuffer(immediateCommandBuffer, &beginInfo);

		return immediateCommandBuffer;
	}

	void VulkanTransientCommandPool::EndSingleTimeCommand() {
		vkEndCommandBuffer(m_CommandBuffers[m_CommandBuffers.size() - 1]);
	}

	void VulkanTransientCommandPool::Destroy() {
		VulkanCommandPool::Destroy();
	}
}
