#pragma once
#include "Core/Base.h"
#include "GraphicsPipeline.h"

#include "Renderer/Descriptors/VulkanDescriptorPool.h"

namespace Lucy {

	class VulkanGraphicsPipeline : public GraphicsPipeline {
	public:
		VulkanGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo, const Ref<VulkanRenderDevice>& vulkanDevice);
		virtual ~VulkanGraphicsPipeline() = default;

		void RTBind(void* commandBufferHandle) final override;
		void RTRecreate() final override;

		inline VkPipeline GetVulkanHandle() { return m_PipelineHandle; }
		inline VkPipelineLayout GetPipelineLayout() { return m_PipelineLayoutHandle; }
	private:
		void Create(const Ref<VulkanRenderDevice>& vulkanDevice);
		void RTDestroyResource() final override;

		VkVertexInputBindingDescription CreateBindingDescription() const;
		std::vector<VkVertexInputAttributeDescription> CreateAttributeDescription(uint32_t binding);
		VkFormat GetVulkanTypeFromSize(ShaderMemberType type, uint32_t size) const;

		VkPipeline m_PipelineHandle = VK_NULL_HANDLE;
		VkPipelineLayout m_PipelineLayoutHandle = VK_NULL_HANDLE;

		Ref<VulkanDescriptorPool> m_DescriptorPool;
	};
}