#include "lypch.h"
#include "VulkanComputePipeline.h"

#include "Renderer/Renderer.h"
#include "Renderer/Device/VulkanRenderDevice.h"
#include "Renderer/Descriptors/VulkanDescriptorSet.h"

#include "Renderer/Shader/VulkanComputeShader.h"
#include "Renderer/Memory/Buffer/PushConstant.h"

#include "Utilities/Utilities.h"

namespace Lucy {

	VulkanComputePipeline::VulkanComputePipeline(const ComputePipelineCreateInfo& createInfo, const Ref<VulkanRenderDevice>& vulkanDevice)
		: ComputePipeline(createInfo), m_VulkanDevice(vulkanDevice) {
		RTCreate();
	}

	void VulkanComputePipeline::RTCreate() {
		const std::vector<VkDescriptorPoolSize> poolSizes = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 }
		};

		VulkanDescriptorPoolCreateInfo poolCreateInfo;
		poolCreateInfo.PoolSizesVector = poolSizes;
		poolCreateInfo.MaxSet = 100;
		poolCreateInfo.PoolFlags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
		poolCreateInfo.LogicalDevice = m_VulkanDevice->GetLogicalDevice();
		m_DescriptorPool = Memory::CreateRef<VulkanDescriptorPool>(poolCreateInfo);

		m_CreateInfo.Shader->RTLoadDescriptors(m_VulkanDevice, m_DescriptorPool);
		const auto& descriptorSetsHandles = m_CreateInfo.Shader->GetDescriptorSetHandles();
		const auto& pushConstants = m_CreateInfo.Shader->GetPushConstants();

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
		descriptorSetLayouts.reserve(descriptorSetsHandles.size());
		for (auto handle : descriptorSetsHandles) {
			const auto& descriptorSet = m_VulkanDevice->AccessResource<VulkanDescriptorSet>(handle);
			descriptorSetLayouts.emplace_back(descriptorSet->GetDescriptorSetLayout());
		}

		std::vector<VkPushConstantRange> pushConstantRanges;
		for (const VulkanPushConstant& pc : pushConstants)
			pushConstantRanges.push_back(pc.GetHandle());
		
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = VulkanAPI::PipelineLayoutCreateInfo((uint32_t)descriptorSetLayouts.size(), descriptorSetLayouts.data(), (uint32_t)pushConstantRanges.size(), pushConstantRanges.data());

		LUCY_VK_ASSERT(vkCreatePipelineLayout(m_VulkanDevice->GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayoutHandle));

		VkComputePipelineCreateInfo pipelineInfo = VulkanAPI::ComputePipelineCreateInfo(m_PipelineLayoutHandle, m_CreateInfo.Shader->As<VulkanComputeShader>()->GetShaderStageInfo());
		LUCY_VK_ASSERT(vkCreateComputePipelines(m_VulkanDevice->GetLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_PipelineHandle));
#ifdef LUCY_DEBUG
		LUCY_INFO("Vulkan compute pipeline '{0}' created successfully!", m_CreateInfo.Shader->GetName());
#endif
	}

	void VulkanComputePipeline::RTBind(void* commandBufferHandle) {
		vkCmdBindPipeline((VkCommandBuffer)commandBufferHandle, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineHandle);
	}

	void VulkanComputePipeline::RTDispatch(void* commandBufferHandle, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
		vkCmdDispatch((VkCommandBuffer)commandBufferHandle, groupCountX, groupCountY, groupCountZ);
	}

	void VulkanComputePipeline::RTRecreate() {
		RTDestroyResource();
		RTCreate();
	}

	void VulkanComputePipeline::RTDestroyResource() {
		m_DescriptorPool->RTDestroyResource();
		vkDestroyPipelineLayout(m_VulkanDevice->GetLogicalDevice(), m_PipelineLayoutHandle, nullptr);
		vkDestroyPipeline(m_VulkanDevice->GetLogicalDevice(), m_PipelineHandle, nullptr);
	}
}