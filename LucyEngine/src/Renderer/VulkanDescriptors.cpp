#include "lypch.h"
#include "VulkanDescriptors.h"

#include "Context/VulkanSwapChain.h"
#include "Context/VulkanDevice.h"

#include "Renderer/Renderer.h"

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

	VulkanDescriptorSet::VulkanDescriptorSet(const VulkanDescriptorSetCreateInfo& createInfo)
		: m_CreateInfo(createInfo) {
		Create();
	}

	void VulkanDescriptorSet::Bind(const VulkanDescriptorSetBindInfo& bindInfo) {
		vkCmdBindDescriptorSets(bindInfo.CommandBuffer, bindInfo.PipelineBindPoint, bindInfo.PipelineLayout, m_CreateInfo.SetIndex, 1, &m_DescriptorSets[VulkanSwapChain::Get().GetCurrentFrameIndex()], 0, nullptr);
	}

	void VulkanDescriptorSet::Create() {
		const VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		const uint32_t maxFramesInFlight = swapChain.GetMaxFramesInFlight();
		VkDevice device = VulkanDevice::Get().GetLogicalDevice();

		std::vector<VkDescriptorSetLayout> tempVector(maxFramesInFlight, m_CreateInfo.Layout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.pSetLayouts = tempVector.data();
		allocInfo.descriptorPool = m_CreateInfo.Pool->GetVulkanHandle();
		allocInfo.descriptorSetCount = maxFramesInFlight;

		//because its stack allocated
		VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescriptorCountAllocInfo{};

		if (m_CreateInfo.SizeOfVariableCounting != 0) {
			uint32_t variableDescCounts[] = { m_CreateInfo.SizeOfVariableCounting, m_CreateInfo.SizeOfVariableCounting, m_CreateInfo.SizeOfVariableCounting };

			variableDescriptorCountAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
			variableDescriptorCountAllocInfo.descriptorSetCount = maxFramesInFlight;
			variableDescriptorCountAllocInfo.pDescriptorCounts = variableDescCounts;

			allocInfo.pNext = &variableDescriptorCountAllocInfo;
		} else {
			allocInfo.pNext = nullptr;
		}

		m_DescriptorSets.resize(maxFramesInFlight);
		LUCY_VK_ASSERT(vkAllocateDescriptorSets(device, &allocInfo, m_DescriptorSets.data()));
	}
}
