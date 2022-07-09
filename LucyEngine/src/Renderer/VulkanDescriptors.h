#pragma once

#include "vulkan/vulkan.h"

namespace Lucy {

	constexpr uint32_t MAX_DYNAMICALLY_ALLOCATED_BUFFER = 1000u;

	enum class GlobalDescriptorSets : uint8_t {
		Camera = 0,
		Material = 1
	};

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

	struct VulkanDescriptorSetCreateInfo {
		VkDescriptorSetLayout Layout = VK_NULL_HANDLE;
		Ref<VulkanDescriptorPool> Pool = nullptr;
		uint32_t SetIndex;
	};

	struct VulkanDescriptorSetBindInfo {
		VkCommandBuffer CommandBuffer;
		VkPipelineBindPoint PipelineBindPoint;
		VkPipelineLayout PipelineLayout;
	};

	class VulkanDescriptorSet {
	public:
		VulkanDescriptorSet(const VulkanDescriptorSetCreateInfo& createInfo);
		~VulkanDescriptorSet() = default;

		void Bind(const VulkanDescriptorSetBindInfo& bindInfo);

		inline VkDescriptorSet GetSetBasedOffCurrentFrame(uint32_t currentFrame) const noexcept { return m_DescriptorSets[currentFrame]; }
	private:
		void Create();

		std::vector<VkDescriptorSet> m_DescriptorSets;
		VulkanDescriptorSetCreateInfo m_CreateInfo;
	};
}

