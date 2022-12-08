#include "lypch.h"
#include "VulkanComputePipeline.h"

#include "Renderer/Renderer.h"
#include "Renderer/Shader/VulkanComputeShader.h"
#include "Renderer/Context/VulkanContextDevice.h"

namespace Lucy {

	VulkanComputePipeline::VulkanComputePipeline(const ComputePipelineCreateInfo& createInfo)
		: ComputePipeline(createInfo) {
		Renderer::EnqueueToRenderThread([=]() {
			Create();
		});
	}

	void VulkanComputePipeline::Create() {
		const std::vector<VkDescriptorPoolSize> poolSizes = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 }
		};

		VulkanDescriptorPoolCreateInfo poolCreateInfo{};
		poolCreateInfo.PoolSizesVector = poolSizes;
		poolCreateInfo.MaxSet = 100;
		poolCreateInfo.PoolFlags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
		m_DescriptorPool = Memory::CreateRef<VulkanDescriptorPool>(poolCreateInfo);

		VulkanContextPipelineUtils::ParseVulkanDescriptorSets(m_DescriptorSetLayouts, m_CreateInfo.Shader, m_DescriptorPool, m_DescriptorSets, m_PushConstants);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = (uint32_t)m_DescriptorSetLayouts.size();
		pipelineLayoutInfo.pSetLayouts = m_DescriptorSetLayouts.data();

		std::vector<VkPushConstantRange> pushConstantRanges;
		for (VulkanPushConstant& pc : m_PushConstants)
			pushConstantRanges.push_back(pc.GetHandle());

		pipelineLayoutInfo.pushConstantRangeCount = (uint32_t)pushConstantRanges.size();
		pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

		VkDevice device = VulkanContextDevice::Get().GetLogicalDevice();
		LUCY_VK_ASSERT(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_PipelineLayoutHandle));

		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.layout = m_PipelineLayoutHandle;
		pipelineInfo.stage = m_CreateInfo.Shader.As<VulkanComputeShader>()->GetShaderStageInfo();

		LUCY_VK_ASSERT(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_PipelineHandle));
	}

	void VulkanComputePipeline::Bind(void* commandBufferHandle) {
		vkCmdBindPipeline((VkCommandBuffer)commandBufferHandle, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineHandle);
	}

	void VulkanComputePipeline::Dispatch(void* commandBufferHandle, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
		vkCmdDispatch((VkCommandBuffer)commandBufferHandle, groupCountX, groupCountY, groupCountZ);
	}

	void VulkanComputePipeline::Recreate(uint32_t width, uint32_t height) {
		//TODO: Do nothing
	}

	void VulkanComputePipeline::Destroy() {
		VkDevice device = VulkanContextDevice::Get().GetLogicalDevice();

		for (const auto& set : m_DescriptorSets)
			set->Destroy();

		for (auto& descriptorSetLayout : m_DescriptorSetLayouts)
			vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		m_DescriptorPool->Destroy();
		vkDestroyPipelineLayout(device, m_PipelineLayoutHandle, nullptr);
		vkDestroyPipeline(device, m_PipelineHandle, nullptr);
	}
}