#include "lypch.h"
#include "VulkanComputePipeline.h"

#include "Renderer/Renderer.h"
#include "Renderer/Device/VulkanRenderDevice.h"
#include "Renderer/Descriptors/VulkanDescriptorSet.h"

#include "../Context/VulkanContext.h"

#include "Renderer/Shader/VulkanComputeShader.h"
#include "Renderer/Memory/Buffer/PushConstant.h"

namespace Lucy {

	VulkanComputePipeline::VulkanComputePipeline(const ComputePipelineCreateInfo& createInfo, const Ref<VulkanRenderDevice>& vulkanDevice)
		: ComputePipeline(createInfo) {
		Renderer::EnqueueToRenderCommandQueue([=](const auto& device) {
			const auto& vulkanDevice = device->As<VulkanRenderDevice>();
			Create(vulkanDevice);
		});
	}

	void VulkanComputePipeline::Create(const Ref<VulkanRenderDevice>& vulkanDevice) {
		VkDevice logicalDevice = vulkanDevice->GetLogicalDevice();

		const auto& shader = GetShader();

		const auto& reflectPushConstants = shader->GetShaderPushConstants();
		for (auto& pc : reflectPushConstants)
			AddPushConstant(pc);

		m_DescriptorSetHandles = vulkanDevice->GetResourceBindingHandles(shader);
		const auto& pushConstants = GetPipelineConstants();

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

		uint32_t highestSetIndex = 0;

		for (RenderDeviceResourceHandle handle : m_DescriptorSetHandles) {
			const auto& descriptorSet = vulkanDevice->AccessResource<VulkanDescriptorSet>(handle);
			highestSetIndex = std::max(highestSetIndex, descriptorSet->GetSetIndex());
		}

		descriptorSetLayouts.resize(highestSetIndex + 1, VK_NULL_HANDLE);

		for (RenderDeviceResourceHandle handle : m_DescriptorSetHandles) {
			const auto& descriptorSet = vulkanDevice->AccessResource<VulkanDescriptorSet>(handle);

			const uint32_t setIndex = descriptorSet->GetSetIndex();
			descriptorSetLayouts[setIndex] = descriptorSet->GetDescriptorSetLayout();
		}

		for (uint32_t i = 0; i < descriptorSetLayouts.size(); i++)
			LUCY_ASSERT(descriptorSetLayouts[i] != VK_NULL_HANDLE, "Missing descriptor set layout for set index {0}", i);

		std::vector<VkPushConstantRange> pushConstantRanges;
		for (const PipelineConstant& pc : pushConstants)
			pushConstantRanges.push_back(pc.GetHandle());
		
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = VulkanAPI::PipelineLayoutCreateInfo((uint32_t)descriptorSetLayouts.size(), descriptorSetLayouts.data(), (uint32_t)pushConstantRanges.size(), pushConstantRanges.data());

		LUCY_VK_ASSERT(vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &m_PipelineLayoutHandle));

		VkComputePipelineCreateInfo pipelineInfo = VulkanAPI::ComputePipelineCreateInfo(m_PipelineLayoutHandle, m_CreateInfo.Shader->As<VulkanComputeShader>()->GetShaderInfo());
		LUCY_VK_ASSERT(vkCreateComputePipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_PipelineHandle));
		LUCY_INFO("Vulkan compute pipeline '{0}' created successfully!", m_CreateInfo.Shader->GetName());
#ifdef LUCY_DEBUG
		
		std::string objectName = std::format("{0} Compute Pipeline", GetDebugName());

		VkDebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = VK_OBJECT_TYPE_PIPELINE;
		nameInfo.objectHandle = reinterpret_cast<uint64_t>(m_PipelineHandle);
		nameInfo.pObjectName = objectName.c_str();

		VulkanExternalFuncLinkage::vkSetDebugUtilsObjectNameEXT(logicalDevice, &nameInfo);
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
		Renderer::EnqueueToRenderCommandQueue([&](const auto& device) {
			const auto& vulkanDevice = device->As<VulkanRenderDevice>();
			Create(vulkanDevice);
		});
	}

	void VulkanComputePipeline::RTDestroyResource() {
		Pipeline::RTDestroyResource();

		Renderer::EnqueueToRenderCommandQueue([=](const auto& device) {
			const auto& vulkanDevice = device->As<VulkanRenderDevice>();

			vkDestroyPipelineLayout(vulkanDevice->GetLogicalDevice(), m_PipelineLayoutHandle, nullptr);
			vkDestroyPipeline(vulkanDevice->GetLogicalDevice(), m_PipelineHandle, nullptr);
		});
	}
}