#pragma once

#include "DescriptorSet.h"
#include "vulkan/vulkan.h"

#include "Renderer/Descriptors/VulkanDescriptorPool.h"

namespace Lucy {

	constexpr uint32_t MAX_DYNAMIC_DESCRIPTOR_COUNT = 1000u;
	constexpr uint32_t MAX_DYNAMICALLY_ALLOCATED_BUFFER_SIZE = MAX_DYNAMIC_DESCRIPTOR_COUNT * 10u; //10 kilobytes

	struct VulkanDescriptorSetCreateInfo {
		VkDescriptorSetLayout Layout = VK_NULL_HANDLE;
		Ref<VulkanDescriptorPool> Pool = nullptr;
	};

	struct VulkanDescriptorSetBindInfo {
		VkCommandBuffer CommandBuffer;
		VkPipelineBindPoint PipelineBindPoint;
		VkPipelineLayout PipelineLayout;
	};

	class VulkanDescriptorSet : public DescriptorSet {
	public:
		VulkanDescriptorSet(const DescriptorSetCreateInfo& createInfo);
		virtual ~VulkanDescriptorSet() = default;

		void Bind(const VulkanDescriptorSetBindInfo& bindInfo);
		void Update() override;
	private:
		void Create();

		std::vector<VkDescriptorSet> m_DescriptorSets;
	};
}

