#pragma once

#include "Pipeline.h"
#include "Renderer/Buffer/Vulkan/VulkanUniformBuffer.h"

namespace Lucy {

	class VulkanPipeline : public Pipeline {
	public:
		VulkanPipeline(const PipelineSpecification& specs);
		virtual ~VulkanPipeline() = default;

		void Destroy();
		void Recreate(float sizeX, float sizeY);

		void BeginVirtual() override;
		void EndVirtual() override;

		inline VkPipeline GetVulkanHandle() { return m_Pipeline; }
		inline VkPipelineLayout GetPipelineLayout() { return m_PipelineLayout; }
	private:
		void Create();

		VkVertexInputBindingDescription CreateBindingDescription();
		std::vector<VkVertexInputAttributeDescription> CreateAttributeDescription(uint32_t binding);
		VkFormat GetVulkanTypeFromSize(ShaderDataSize size);

		std::vector<VkDescriptorPoolSize> CreateDescriptorPoolSizes();
		void ParseUniformBuffers();

		VkPipeline m_Pipeline = VK_NULL_HANDLE;
		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;

		static RefLucy<VulkanDescriptorPool> s_DescriptorPool;
		std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;
	};
}