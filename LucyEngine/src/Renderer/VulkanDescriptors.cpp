#include "lypch.h"
#include "VulkanDescriptors.h"

#include "Renderer/Renderer.h"
#include "Context/VulkanSwapChain.h"
#include "Context/VulkanDevice.h"

namespace Lucy {

	VulkanDescriptorPool::VulkanDescriptorPool(VulkanDescriptorPoolSpecifications& specs)
		: m_Specs(specs) {
		Create();
	}

	void VulkanDescriptorPool::Create() {
		VkDescriptorPoolCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		info.poolSizeCount = m_Specs.PoolSizesVector.size();
		info.pPoolSizes = m_Specs.PoolSizesVector.data();
		info.maxSets = m_Specs.MaxSet;
		info.flags = m_Specs.PoolFlags;

		VkDevice device = VulkanDevice::Get().GetLogicalDevice();
		LUCY_VK_ASSERT(vkCreateDescriptorPool(device, &info, nullptr, &m_DescriptorPool));
	}

	void VulkanDescriptorPool::Destroy() {
		VkDevice device = VulkanDevice::Get().GetLogicalDevice();
		vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
	}

	VulkanDescriptorSet::VulkanDescriptorSet(VulkanDescriptorSetSpecifications& specs)
		: m_Specs(specs) {
		Create();
	}

	void VulkanDescriptorSet::Create() {
		constexpr uint32_t maxFramesInFlight = VulkanSwapChain::MAX_FRAMES_IN_FLIGHT;
		VkDevice device = VulkanDevice::Get().GetLogicalDevice();

		std::vector<VkDescriptorSetLayout> tempVector(maxFramesInFlight, m_Specs.Layout);

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.pSetLayouts = tempVector.data();
		allocInfo.descriptorPool = m_Specs.Pool->GetVulkanHandle();
		allocInfo.descriptorSetCount = maxFramesInFlight;

		m_DescriptorSets.resize(maxFramesInFlight);
		LUCY_VK_ASSERT(vkAllocateDescriptorSets(device, &allocInfo, m_DescriptorSets.data()));
	}
}
