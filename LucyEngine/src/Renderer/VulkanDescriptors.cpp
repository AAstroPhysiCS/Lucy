#include "lypch.h"
#include "VulkanDescriptors.h"

#include "Renderer/Renderer.h"
#include "Context/VulkanDevice.h"

namespace Lucy {

	VulkanDescriptorPool::VulkanDescriptorPool(VulkanDescriptorPoolSpecifications& specs)
		: m_Specs(specs) {
		Renderer::Submit([this]() {
			Create();
		});
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
		Renderer::Submit([this]() {
			Create();
		});
	}

	void VulkanDescriptorSet::Create() {
		VkDevice device = VulkanDevice::Get().GetLogicalDevice();

		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		createInfo.bindingCount = m_Specs.LayoutBindings.size();
		createInfo.pBindings = m_Specs.LayoutBindings.data();
		LUCY_VK_ASSERT(vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &m_Layout));

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.pSetLayouts = &m_Layout;
		allocInfo.descriptorPool = m_Specs.Pool->GetVulkanHandle();
		allocInfo.descriptorSetCount = 1;

		LUCY_VK_ASSERT(vkAllocateDescriptorSets(device, &allocInfo, &m_DescriptorSet));
	}

	void VulkanDescriptorSet::Destroy() {
		VkDevice device = VulkanDevice::Get().GetLogicalDevice();
		vkDestroyDescriptorSetLayout(device, m_Layout, nullptr);
	}
}
