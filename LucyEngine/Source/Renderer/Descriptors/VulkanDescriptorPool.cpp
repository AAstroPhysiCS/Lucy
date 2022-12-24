#include "lypch.h"
#include "VulkanDescriptorPool.h"

#include "Renderer/Renderer.h"
#include "Renderer/Context/VulkanContextDevice.h"

namespace Lucy {

	VulkanDescriptorPool::VulkanDescriptorPool(const VulkanDescriptorPoolCreateInfo& createInfo)
		: m_CreateInfo(createInfo) {
		Create();
	}

	void VulkanDescriptorPool::Create() {
		VkDescriptorPoolCreateInfo info = VulkanAPI::DescriptorPoolCreateInfo((uint32_t)m_CreateInfo.PoolSizesVector.size(), m_CreateInfo.PoolSizesVector.data(), 
																			  m_CreateInfo.MaxSet, m_CreateInfo.PoolFlags);

		VkDevice device = VulkanContextDevice::Get().GetLogicalDevice();
		LUCY_VK_ASSERT(vkCreateDescriptorPool(device, &info, nullptr, &m_DescriptorPool));
	}

	void VulkanDescriptorPool::Destroy() {
		VkDevice device = VulkanContextDevice::Get().GetLogicalDevice();
		vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
	}
}