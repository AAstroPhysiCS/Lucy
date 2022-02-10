#pragma once

#include "vulkan/vulkan.h"

namespace Lucy {

	struct VulkanDescriptorPoolSpecifications {
		std::vector<VkDescriptorPoolSize> PoolSizesVector;
		VkDescriptorPoolCreateFlags PoolFlags = 0;
		uint32_t MaxSet = 0;
	};

	class VulkanDescriptorPool {
	public:
		VulkanDescriptorPool(VulkanDescriptorPoolSpecifications& specs);
		~VulkanDescriptorPool() = default;

		void Destroy();

		inline VkDescriptorPool GetVulkanHandle() const { return m_DescriptorPool; }
	private:
		void Create();

		VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
		VulkanDescriptorPoolSpecifications m_Specs;
	};

	struct VulkanDescriptorSetSpecifications {
		std::vector<VkDescriptorSetLayoutBinding> LayoutBindings;
		RefLucy<VulkanDescriptorPool> Pool = nullptr;
	};

	class VulkanDescriptorSet {
	public:
		VulkanDescriptorSet(VulkanDescriptorSetSpecifications& specs);
		~VulkanDescriptorSet() = default;
	private:
		void Create();
		void Destroy();

		VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
		VulkanDescriptorSetSpecifications m_Specs;
	};
}

