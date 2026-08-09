#pragma once
#include "Core/Base.h"
#include "GraphicsPipeline.h"

#include "Renderer/Descriptors/VulkanDescriptorPool.h"
#include "Renderer/Memory/Buffer/PushConstant.h"

namespace Lucy {

	class VulkanGraphicsPipeline : public GraphicsPipeline {
	public:
		VulkanGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo, const Ref<Shader>& shader, const Ref<VulkanRenderDevice>& vulkanDevice);
		virtual ~VulkanGraphicsPipeline() = default;

		VulkanGraphicsPipeline(const VulkanGraphicsPipeline&) = delete;
		VulkanGraphicsPipeline& operator=(const VulkanGraphicsPipeline&) = delete;
		VulkanGraphicsPipeline(VulkanGraphicsPipeline&&) = delete;
		VulkanGraphicsPipeline& operator=(VulkanGraphicsPipeline&&) = delete;

		void RTBind(void* commandBufferHandle) final override;
		void RTRecreate(Ref<Shader> newShader) final override;

		VkPipeline GetVulkanHandle() { return m_PipelineHandle; }
		VkPipelineLayout GetPipelineLayout() { return m_PipelineLayoutHandle; }

		const std::vector<RenderDeviceResourceHandle>& GetDescriptorSetHandles() const { return m_DescriptorSetHandles; }
	private:
		void Create(const Ref<VulkanRenderDevice>& vulkanDevice);
		void RTDestroyResource(RenderDevice* device) final override;

		VkVertexInputBindingDescription CreateBindingDescription() const;
		std::vector<VkVertexInputAttributeDescription> CreateAttributeDescription(uint32_t binding);
		VkFormat GetVulkanTypeFromSize(ShaderMemberType type, uint32_t size) const;

		VkPipeline m_PipelineHandle = VK_NULL_HANDLE;
		VkPipelineLayout m_PipelineLayoutHandle = VK_NULL_HANDLE;

		std::vector<RenderDeviceResourceHandle> m_DescriptorSetHandles;
	};
}