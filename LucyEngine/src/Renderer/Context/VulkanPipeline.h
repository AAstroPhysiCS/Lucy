#pragma once

#include "Pipeline.h"
#include "Renderer/Buffer/Vulkan/VulkanUniformBuffer.h"

namespace Lucy {

	class VulkanPipeline : public Pipeline {
	public:
		VulkanPipeline(const PipelineSpecification& specs);
		virtual ~VulkanPipeline() = default;

		void BeginVirtual() override;
		void EndVirtual() override;
		void Recreate() override;

		void Destroy() override;

		inline VkPipeline GetVulkanHandle() { return m_Pipeline; }
		inline VkPipelineLayout GetPipelineLayout() { return m_PipelineLayout; }
		inline std::vector<VulkanDescriptorSet>& GetIndividualSetsToBind() { return m_IndividualSets; }
	private:
		void Create();

		VkVertexInputBindingDescription CreateBindingDescription();
		std::vector<VkVertexInputAttributeDescription> CreateAttributeDescription(uint32_t binding);
		VkFormat GetVulkanTypeFromSize(ShaderDataSize size);

		std::vector<VkDescriptorPoolSize> CreateDescriptorPoolSizes();
		void ParseUniformBuffers();

		VkPipeline m_Pipeline = VK_NULL_HANDLE;
		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;

		RefLucy<VulkanDescriptorPool> m_DescriptorPool;
		std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;
		std::vector<VulkanDescriptorSet> m_IndividualSets; //meaning that only distinct sets are being stored (used for binding sets)
	};
}