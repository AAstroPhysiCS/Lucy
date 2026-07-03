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

		const std::vector<VkDescriptorPoolSize> poolSizes = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_DYNAMIC_DESCRIPTOR_COUNT },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_DYNAMIC_DESCRIPTOR_COUNT },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, MAX_DYNAMIC_DESCRIPTOR_COUNT },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_DYNAMIC_DESCRIPTOR_COUNT }
		};

		VulkanDescriptorPoolCreateInfo poolCreateInfo;
		poolCreateInfo.PoolSizesVector = poolSizes;
		poolCreateInfo.MaxSet = 10;
		poolCreateInfo.PoolFlags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
		poolCreateInfo.LogicalDevice = logicalDevice;
		m_DescriptorPool = Memory::CreateRef<VulkanDescriptorPool>(poolCreateInfo);

		RTLoadDescriptors(vulkanDevice);
		const auto& descriptorSetsHandles = GetDescriptorSetHandles();
		const auto& pushConstants = GetPipelineConstants();

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
		descriptorSetLayouts.reserve(descriptorSetsHandles.size());
		for (auto handle : descriptorSetsHandles) {
			const auto& descriptorSet = vulkanDevice->AccessResource<VulkanDescriptorSet>(handle);
			descriptorSetLayouts.emplace_back(descriptorSet->GetDescriptorSetLayout());
		}

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

	void VulkanComputePipeline::RTLoadDescriptors(const Ref<RenderDevice>& vulkanDevice) {
		const auto& shader = GetShader();

		const auto& reflectPushConstants = shader->GetShaderPushConstants();
		const auto& reflectUniformBlockMaps = shader->GetShaderUniformBlockMap();

		for (const auto& [set, info] : reflectUniformBlockMaps) {
			DescriptorSetCreateInfo createInfo{
				.SetIndex = set,
				.ShaderVariables = info,
			};
			RenderResourceHandle descriptorSetHandle = vulkanDevice->CreateDescriptorSet(createInfo);
			const auto& descriptorSet = vulkanDevice->AccessResource<VulkanDescriptorSet>(descriptorSetHandle);
			descriptorSet->RTBake(m_DescriptorPool);
			AddDescriptorSetHandle(descriptorSetHandle); //maybe just store the handle?
		}

		for (auto& pc : reflectPushConstants)
			AddPushConstant(pc);
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

			m_DescriptorPool->RTDestroyResource();
			m_DescriptorPool = nullptr;
			vkDestroyPipelineLayout(vulkanDevice->GetLogicalDevice(), m_PipelineLayoutHandle, nullptr);
			vkDestroyPipeline(vulkanDevice->GetLogicalDevice(), m_PipelineHandle, nullptr);
		});
	}
}