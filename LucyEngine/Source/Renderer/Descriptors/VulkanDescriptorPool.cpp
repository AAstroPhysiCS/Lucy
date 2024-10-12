#include "lypch.h"
#include "VulkanDescriptorPool.h"

#include "Renderer/Renderer.h"
#include "Renderer/Device/VulkanRenderDevice.h"

namespace Lucy {

	VulkanDescriptorPool::VulkanDescriptorPool(const VulkanDescriptorPoolCreateInfo& createInfo)
		: m_CreateInfo(createInfo) {
		RTCreate();
	}

	void VulkanDescriptorPool::RTCreate() {
		VkDescriptorPoolCreateInfo info = VulkanAPI::DescriptorPoolCreateInfo((uint32_t)m_CreateInfo.PoolSizesVector.size(), m_CreateInfo.PoolSizesVector.data(), 
																			  m_CreateInfo.MaxSet, m_CreateInfo.PoolFlags);

		LUCY_VK_ASSERT(vkCreateDescriptorPool(m_CreateInfo.LogicalDevice, &info, nullptr, &m_DescriptorPool));
	}

	void VulkanDescriptorPool::RTDestroyResource() {
		LUCY_ASSERT(Renderer::IsOnRenderThread());
		vkDestroyDescriptorPool(m_CreateInfo.LogicalDevice, m_DescriptorPool, nullptr);
	}
}