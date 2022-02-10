#pragma once

#include "Pipeline.h"
#include "Renderer/VulkanDescriptors.h"

namespace Lucy {

	class VulkanPipeline : public Pipeline {
	public:
		VulkanPipeline(PipelineSpecification& specs);
		virtual ~VulkanPipeline() = default;

		void Destroy();
		void Recreate(float sizeX, float sizeY);

		void BeginVirtual();
		void EndVirtual();

		inline VkPipeline GetVulkanHandle() { return m_Pipeline; }
	private:
		void Create();

		VkVertexInputBindingDescription CreateBindingDescription();
		std::vector<VkVertexInputAttributeDescription> CreateAttributeDescription(uint32_t binding);
		VkFormat GetVulkanTypeFromSize(ShaderDataSize size);

		std::vector<VkDescriptorPoolSize>& CreateDescriptorPoolSizes();
		std::vector<VkDescriptorSetLayout> CreateDescriptorSets();

		VkPipeline m_Pipeline = VK_NULL_HANDLE;
		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;

		static RefLucy<VulkanDescriptorPool> s_DescriptorPool;
		std::vector<VulkanDescriptorSet> m_DescriptorSets;
	};
}