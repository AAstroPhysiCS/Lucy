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

		inline VkDescriptorPool GetVulkanHandle() const noexcept { return m_DescriptorPool; }
	private:
		void Create();

		VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
		VulkanDescriptorPoolSpecifications m_Specs;
	};

	struct VulkanDescriptorSetSpecifications {
		VkDescriptorSetLayout Layout = VK_NULL_HANDLE;
		RefLucy<VulkanDescriptorPool> Pool = nullptr;
	};

	class VulkanDescriptorSet {
	public:
		VulkanDescriptorSet(VulkanDescriptorSetSpecifications& specs);
		~VulkanDescriptorSet() = default;

		inline VkDescriptorSet GetSetBasedOffFrameIndex(uint32_t currentFrame) const noexcept { return m_DescriptorSets[currentFrame]; }
	private:
		void Create();

		std::vector<VkDescriptorSet> m_DescriptorSets;
		VulkanDescriptorSetSpecifications m_Specs;
	};
}

