#pragma once

#include "ComputePipeline.h"
#include "Renderer/Descriptors/VulkanDescriptorPool.h"

namespace Lucy {

	class VulkanComputePipeline : public ComputePipeline {
	public:
		VulkanComputePipeline(const Ref<ComputeShader>& shader);
		virtual ~VulkanComputePipeline() = default;

		void Bind(void* commandBufferHandle) final override;
		void Dispatch(void* commandBufferHandle, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) final override;
		void Destroy() final override;
	private:
		void Create();
		void ParseDescriptorSets() final override;

		VkPipeline m_PipelineHandle = VK_NULL_HANDLE;
		VkPipelineLayout m_PipelineLayoutHandle = VK_NULL_HANDLE;

		Ref<VulkanDescriptorPool> m_DescriptorPool;
		std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;
		std::vector<VulkanPushConstant> m_PushConstants;
	};
}