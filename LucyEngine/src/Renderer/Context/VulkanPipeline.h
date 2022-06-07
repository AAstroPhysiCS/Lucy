#pragma once

#include "Pipeline.h"
#include "Renderer/Buffer/Vulkan/VulkanUniformBuffer.h"

namespace Lucy {

	class VulkanPipeline : public Pipeline {
	public:
		VulkanPipeline(const PipelineSpecification& specs);
		virtual ~VulkanPipeline() = default;

		void Bind(PipelineBindInfo bindInfo) override;
		void Unbind() override;
		void Destroy() override;
		void Recreate(uint32_t width, uint32_t height);

		inline VkPipeline GetVulkanHandle() { return m_PipelineHandle; }
		inline VkPipelineLayout GetPipelineLayout() { return m_PipelineLayoutHandle; }
		inline std::vector<VulkanDescriptorSet> GetIndividualSetsToBind() { return m_IndividualSets; }
	private:
		void Create();

		VkVertexInputBindingDescription CreateBindingDescription();
		std::vector<VkVertexInputAttributeDescription> CreateAttributeDescription(uint32_t binding);
		VkFormat GetVulkanTypeFromSize(ShaderDataSize size);

		std::vector<VkDescriptorPoolSize> CreateDescriptorPoolSizes();
		void ParseUniformBuffers();

		VkPipeline m_PipelineHandle = VK_NULL_HANDLE;
		VkPipelineLayout m_PipelineLayoutHandle = VK_NULL_HANDLE;

		RefLucy<VulkanDescriptorPool> m_DescriptorPool;
		std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;
		std::vector<VulkanDescriptorSet> m_IndividualSets; //meaning that only distinct sets are being stored (used for binding sets)
	};
}