#pragma once

#include "ComputePipeline.h"

#include "Renderer/Descriptors/VulkanDescriptorPool.h"

namespace Lucy {

	class VulkanComputePipeline : public ComputePipeline {
	public:
		VulkanComputePipeline(const ComputePipelineCreateInfo& createInfo, const Ref<Shader>& shader, const Ref<VulkanRenderDevice>& vulkanDevice);
		virtual ~VulkanComputePipeline() = default;

		VulkanComputePipeline(const VulkanComputePipeline&) = delete;
		VulkanComputePipeline& operator=(const VulkanComputePipeline&) = delete;
		VulkanComputePipeline(VulkanComputePipeline&&) = delete;
		VulkanComputePipeline& operator=(VulkanComputePipeline&&) = delete;

		inline VkPipeline GetVulkanHandle() { return m_PipelineHandle; }
		inline VkPipelineLayout GetPipelineLayout() { return m_PipelineLayoutHandle; }

		const std::vector<RenderDeviceResourceHandle>& GetDescriptorSetHandles() const { return m_DescriptorSetHandles; }

		void RTBind(void* commandBufferHandle) final override;
		void RTRecreate(Ref<Shader> newShader) final override;
		void RTDispatch(void* commandBufferHandle, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) final override;
	private:
		void Create(const Ref<VulkanRenderDevice>& vulkanDevice);
		void RTDestroyResource(RenderDevice* device) final override;

		VkPipeline m_PipelineHandle = VK_NULL_HANDLE;
		VkPipelineLayout m_PipelineLayoutHandle = VK_NULL_HANDLE;

		std::vector<RenderDeviceResourceHandle> m_DescriptorSetHandles;
	};
}