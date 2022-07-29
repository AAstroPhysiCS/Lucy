#pragma once

#include "DescriptorSet.h"
#include "vulkan/vulkan.h"

#include "Renderer/Descriptors/VulkanDescriptorPool.h"

namespace Lucy {

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
		void Update() final override;
	private:
		void Create();

		std::vector<VkDescriptorSet> m_DescriptorSets;
	};
}

