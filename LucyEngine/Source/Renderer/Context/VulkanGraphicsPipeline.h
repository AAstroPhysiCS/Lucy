#pragma once

#include "GraphicsPipeline.h"

#include "Renderer/Descriptors/VulkanDescriptorPool.h"

namespace Lucy {

	class VulkanGraphicsPipeline : public GraphicsPipeline {
	public:
		VulkanGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo);
		virtual ~VulkanGraphicsPipeline() = default;

		void Bind(void* commandBufferHandle) final override;
		void Destroy() final override;
		void Recreate(uint32_t width, uint32_t height) final override;

		inline VkPipeline GetVulkanHandle() { return m_PipelineHandle; }
		inline VkPipelineLayout GetPipelineLayout() { return m_PipelineLayoutHandle; }
	private:
		void Create();

		VkVertexInputBindingDescription CreateBindingDescription();
		std::vector<VkVertexInputAttributeDescription> CreateAttributeDescription(uint32_t binding);
		VkFormat GetVulkanTypeFromSize(ShaderDataSize size);

		VkPipeline m_PipelineHandle = VK_NULL_HANDLE;
		VkPipelineLayout m_PipelineLayoutHandle = VK_NULL_HANDLE;

		Ref<VulkanDescriptorPool> m_DescriptorPool;
		std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;
	};
}