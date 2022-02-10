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
		std::vector<VkDescriptorSetLayout>& CreateDescriptorSetLayouts();

		VkPipeline m_Pipeline{};
		VkPipelineLayout m_PipelineLayout{};

		static RefLucy<VulkanDescriptorPool> s_DescriptorPool;
	};
}