#pragma once

#include "ComputePipeline.h"

#include "Renderer/Descriptors/VulkanDescriptorPool.h"

namespace Lucy {

	class VulkanComputePipeline : public ComputePipeline {
	public:
		VulkanComputePipeline(const ComputePipelineCreateInfo& createInfo, const Ref<VulkanRenderDevice>& vulkanDevice);
		virtual ~VulkanComputePipeline() = default;

		inline VkPipeline GetVulkanHandle() { return m_PipelineHandle; }
		inline VkPipelineLayout GetPipelineLayout() { return m_PipelineLayoutHandle; }

		void RTBind(void* commandBufferHandle) final override;
		void RTRecreate() final override;
		void RTDispatch(void* commandBufferHandle, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) final override;
	private:
		void RTCreate();
		void RTDestroyResource() final override;

		VkPipeline m_PipelineHandle = VK_NULL_HANDLE;
		VkPipelineLayout m_PipelineLayoutHandle = VK_NULL_HANDLE;

		Ref<VulkanDescriptorPool> m_DescriptorPool = nullptr;
		Ref<VulkanRenderDevice> m_VulkanDevice = nullptr;
	};
}