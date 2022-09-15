#pragma once

#include "GraphicsPipeline.h"

#include "Renderer/Descriptors/VulkanDescriptorPool.h"

namespace Lucy {

	struct VulkanGraphicsPipelineBindInfo {
		VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
		VkPipelineBindPoint PipelineBindPoint;
	};

	class VulkanGraphicsPipeline : public GraphicsPipeline {
	public:
		VulkanGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo);
		virtual ~VulkanGraphicsPipeline() = default;

		void Bind(const VulkanGraphicsPipelineBindInfo& bindInfo);
		void Destroy() final override;
		void Recreate(uint32_t width, uint32_t height) final override;

		inline VkPipeline GetVulkanHandle() { return m_PipelineHandle; }
		inline VkPipelineLayout GetPipelineLayout() { return m_PipelineLayoutHandle; }
	private:
		void Create();
		void ParseDescriptorSets() final override;

		VkVertexInputBindingDescription CreateBindingDescription();
		std::vector<VkVertexInputAttributeDescription> CreateAttributeDescription(uint32_t binding);
		VkFormat GetVulkanTypeFromSize(ShaderDataSize size);

		VkPipeline m_PipelineHandle = VK_NULL_HANDLE;
		VkPipelineLayout m_PipelineLayoutHandle = VK_NULL_HANDLE;

		Ref<VulkanDescriptorPool> m_DescriptorPool;
		std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;
	};
}