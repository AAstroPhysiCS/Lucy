#include "lypch.h"
#include "VulkanDescriptorPool.h"

#include "Renderer/Context/VulkanDevice.h"

namespace Lucy {

	VulkanDescriptorPool::VulkanDescriptorPool(const VulkanDescriptorPoolCreateInfo& createInfo)
		: m_CreateInfo(createInfo) {
		Create();
	}

	void VulkanDescriptorPool::Create() {
		VkDescriptorPoolCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		info.poolSizeCount = m_CreateInfo.PoolSizesVector.size();
		info.pPoolSizes = m_CreateInfo.PoolSizesVector.data();
		info.maxSets = m_CreateInfo.MaxSet;
		info.flags = m_CreateInfo.PoolFlags;

		VkDevice device = VulkanDevice::Get().GetLogicalDevice();
		LUCY_VK_ASSERT(vkCreateDescriptorPool(device, &info, nullptr, &m_DescriptorPool));
	}

	void VulkanDescriptorPool::Destroy() {
		VkDevice device = VulkanDevice::Get().GetLogicalDevice();
		vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
	}
}