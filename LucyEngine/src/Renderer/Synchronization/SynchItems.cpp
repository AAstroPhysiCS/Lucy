#include "lypch.h"
#include "SynchItems.h"
#include "Renderer/Context/VulkanDevice.h"

namespace Lucy {

	Semaphore::Semaphore() {
		VkSemaphoreCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		LUCY_VULKAN_ASSERT(vkCreateSemaphore(VulkanDevice::Get().GetLogicalDevice(), &createInfo, nullptr, &m_Handle));
	}

	void Semaphore::Destroy() {
		vkDestroySemaphore(VulkanDevice::Get().GetLogicalDevice(), m_Handle, nullptr);
	}
	
	Fence::Fence() {
		VkFenceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		LUCY_VULKAN_ASSERT(vkCreateFence(VulkanDevice::Get().GetLogicalDevice(), &createInfo, nullptr, &m_Handle));
	}

	void Fence::Destroy() {
		vkDestroyFence(VulkanDevice::Get().GetLogicalDevice(), m_Handle, nullptr);
	}
}