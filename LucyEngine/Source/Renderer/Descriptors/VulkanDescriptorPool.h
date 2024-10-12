#pragma once

#include "vulkan/vulkan.h"

namespace Lucy {

	struct VulkanDescriptorPoolCreateInfo {
		std::vector<VkDescriptorPoolSize> PoolSizesVector;
		VkDescriptorPoolCreateFlags PoolFlags = 0;
		uint32_t MaxSet = 0;
		VkDevice LogicalDevice = VK_NULL_HANDLE;
	};

	class VulkanDescriptorPool {
	public:
		VulkanDescriptorPool(const VulkanDescriptorPoolCreateInfo& createInfo);
		~VulkanDescriptorPool() = default;

		void RTDestroyResource();

		inline VkDescriptorPool GetVulkanHandle() const noexcept { return m_DescriptorPool; }
	private:
		void RTCreate();

		VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
		VulkanDescriptorPoolCreateInfo m_CreateInfo;
	};
}