#pragma once

#include "Pipeline.h"

#include "Renderer/Descriptors/VulkanDescriptorPool.h"

namespace Lucy {

	class VulkanPipeline : public Pipeline {
	public:
		VulkanPipeline(const PipelineCreateInfo& createInfo);
		virtual ~VulkanPipeline() = default;

		void Bind(PipelineBindInfo bindInfo) override;
		void Destroy() override;
		void Recreate(uint32_t width, uint32_t height);

		inline VkPipeline GetVulkanHandle() { return m_PipelineHandle; }
		inline VkPipelineLayout GetPipelineLayout() { return m_PipelineLayoutHandle; }
	private:
		void Create();

		VkVertexInputBindingDescription CreateBindingDescription();
		std::vector<VkVertexInputAttributeDescription> CreateAttributeDescription(uint32_t binding);
		VkFormat GetVulkanTypeFromSize(ShaderDataSize size);

		std::vector<VkDescriptorPoolSize> CreateDescriptorPoolSizes();
		void ParseBuffers();

		VkPipeline m_PipelineHandle = VK_NULL_HANDLE;
		VkPipelineLayout m_PipelineLayoutHandle = VK_NULL_HANDLE;

		Ref<VulkanDescriptorPool> m_DescriptorPool;
		std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;
	};
}