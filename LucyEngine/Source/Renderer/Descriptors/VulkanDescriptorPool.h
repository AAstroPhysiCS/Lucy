#pragma once

#include "vulkan/vulkan.h"

#include "Renderer/Descriptors/DescriptorType.h"

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

		VulkanDescriptorPool(const VulkanDescriptorPool&) = delete;
		VulkanDescriptorPool& operator=(const VulkanDescriptorPool&) = delete;
		VulkanDescriptorPool(VulkanDescriptorPool&&) = delete;
		VulkanDescriptorPool& operator=(VulkanDescriptorPool&&) = delete;

		void RTDestroyResource();

		inline size_t GetPoolSizeMax(DescriptorType type) const {
			auto it = std::ranges::find_if(m_CreateInfo.PoolSizesVector, [&](const VkDescriptorPoolSize& size) {
				return size.type == ConvertDescriptorType(type);
			});
			if (it != m_CreateInfo.PoolSizesVector.end())
				return it->descriptorCount;
			return 0;
		}
		inline VkDescriptorPool GetVulkanHandle() const noexcept { return m_DescriptorPool; }
	private:
		void RTCreate();

		VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
		VulkanDescriptorPoolCreateInfo m_CreateInfo;
	};
}