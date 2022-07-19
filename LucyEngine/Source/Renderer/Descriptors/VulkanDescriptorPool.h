#pragma once

#include "vulkan/vulkan.h"

namespace Lucy {

	struct VulkanDescriptorPoolCreateInfo {
		std::vector<VkDescriptorPoolSize> PoolSizesVector;
		VkDescriptorPoolCreateFlags PoolFlags = 0;
		uint32_t MaxSet = 0;
	};

	class VulkanDescriptorPool {
	public:
		VulkanDescriptorPool(const VulkanDescriptorPoolCreateInfo& createInfo);
		~VulkanDescriptorPool() = default;

		void Destroy();

		inline VkDescriptorPool GetVulkanHandle() const noexcept { return m_DescriptorPool; }
	private:
		void Create();

		VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
		VulkanDescriptorPoolCreateInfo m_CreateInfo;
	};
}