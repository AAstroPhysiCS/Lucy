#pragma once

#include "ComputePipeline.h"

#include "Renderer/Descriptors/VulkanDescriptorPool.h"

namespace Lucy {

	class VulkanComputePipeline : public ComputePipeline {
	public:
		VulkanComputePipeline(const ComputePipelineCreateInfo& createInfo);
		virtual ~VulkanComputePipeline() = default;

		inline VkPipeline GetVulkanHandle() { return m_PipelineHandle; }
		inline VkPipelineLayout GetPipelineLayout() { return m_PipelineLayoutHandle; }

		void Bind(void* commandBufferHandle) final override;
		void Dispatch(void* commandBufferHandle, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
		void Recreate(uint32_t width, uint32_t height) final override;
		void Destroy() final override;
	private:
		void Create();

		VkPipeline m_PipelineHandle = VK_NULL_HANDLE;
		VkPipelineLayout m_PipelineLayoutHandle = VK_NULL_HANDLE;

		Ref<VulkanDescriptorPool> m_DescriptorPool;
		std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;
	};
}