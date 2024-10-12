#pragma once

#include "DescriptorSet.h"

#include "Renderer/Descriptors/VulkanDescriptorPool.h"

namespace Lucy {

	struct VulkanUniformImageSampler;

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
		VulkanDescriptorSet(const DescriptorSetCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device);
		virtual ~VulkanDescriptorSet() = default;

		void RTBind(const VulkanDescriptorSetBindInfo& bindInfo);
		void RTBake(const Ref<VulkanDescriptorPool>& descriptorPool);
		void RTUpdate() final override;
		
		Ref<VulkanUniformImageSampler> GetVulkanImageSampler(const std::string& imageBufferName);

		inline VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }
	private:
		void RTCreate();
		void RTDestroyResource() final override;

		std::unordered_map<std::string, Ref<VulkanUniformImageSampler>> m_UniformImageSamplers;
		std::vector<VkDescriptorSet> m_DescriptorSets;
		VkDescriptorSetLayout m_DescriptorSetLayout;

		Ref<VulkanRenderDevice> m_VulkanDevice = nullptr;
	};
}

