#include "lypch.h"
#include "VulkanGraphicsPipeline.h"

#include "Renderer/Shader/VulkanGraphicsShader.h"

#include "../VulkanRenderPass.h"

#include "Renderer/Descriptors/VulkanDescriptorSet.h"
#include "Renderer/Device/VulkanRenderDevice.h"

#include "Renderer/Memory/Buffer/PushConstant.h"

namespace Lucy {

	VulkanGraphicsPipeline::VulkanGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo, const Ref<VulkanRenderDevice>& vulkanDevice)
		: GraphicsPipeline(createInfo) {
		Renderer::EnqueueToRenderCommandQueue([&](const auto& device) {
			const auto& vulkanDevice = device->As<VulkanRenderDevice>();
			Create(vulkanDevice);
		});
	}

	void VulkanGraphicsPipeline::Create(const Ref<VulkanRenderDevice>& vulkanDevice) {
		const auto& renderPass = vulkanDevice->AccessResource<RenderPass>(m_CreateInfo.RenderPassHandle)->As<VulkanRenderPass>();

		if (!m_DescriptorPool) {
#if USE_INTEGRATED_GRAPHICS
			const std::vector<VkDescriptorPoolSize> poolSizes = {
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 }
			};
#else
			const std::vector<VkDescriptorPoolSize> poolSizes = {
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 }
			};
#endif
			VulkanDescriptorPoolCreateInfo poolCreateInfo;
			poolCreateInfo.PoolSizesVector = poolSizes;
#if USE_INTEGRATED_GRAPHICS
			poolCreateInfo.MaxSet = 100;
#else
			poolCreateInfo.MaxSet = 100;
#endif
			poolCreateInfo.PoolFlags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
			poolCreateInfo.LogicalDevice = vulkanDevice->GetLogicalDevice();
			m_DescriptorPool = Memory::CreateRef<VulkanDescriptorPool>(poolCreateInfo);
		}

		const auto& bindingDescriptor = CreateBindingDescription();
		const auto& attributeDescriptor = CreateAttributeDescription(bindingDescriptor.binding);

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = VulkanAPI::PipelineVertexInputStateCreateInfo((uint32_t)attributeDescriptor.size(), attributeDescriptor.data(), 1, &bindingDescriptor);
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = VulkanAPI::PipelineInputAssemblyStateCreateInfo(m_CreateInfo.Topology);
		VkPipelineViewportStateCreateInfo viewportState = VulkanAPI::PipelineViewportStateCreateInfo(1, nullptr, 1, nullptr);
		VkPipelineRasterizationDepthClipStateCreateInfoEXT rasterizationDepthClipStateCreateInfo = VulkanAPI::PipelineRasterizationDepthClipStateCreateInfo(m_CreateInfo.DepthConfiguration.DepthClipEnable);
		VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo = VulkanAPI::PipelineRasterizationStateCreateInfo(
			VK_FRONT_FACE_COUNTER_CLOCKWISE, m_CreateInfo.Rasterization.LineWidth,
			m_CreateInfo.Rasterization.PolygonMode, m_CreateInfo.Rasterization.CullingMode, m_CreateInfo.DepthConfiguration.DepthClampEnable, VK_FALSE, VK_FALSE, 0.0f, 0.0f, 0.0f, 
			(const void*)&rasterizationDepthClipStateCreateInfo);

		VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = VulkanAPI::PipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

		VkPipelineColorBlendAttachmentState colorBlendAttachment = 
			VulkanAPI::PipelineColorBlendAttachmentState(m_CreateInfo.BlendConfiguration.BlendEnable,
			m_CreateInfo.BlendConfiguration.SrcColorBlendFactor, m_CreateInfo.BlendConfiguration.DstColorBlendFactor, m_CreateInfo.BlendConfiguration.ColorBlendOp,
			m_CreateInfo.BlendConfiguration.SrcAlphaBlendFactor, m_CreateInfo.BlendConfiguration.DstAlphaBlendFactor, m_CreateInfo.BlendConfiguration.AlphaBlendOp);

		std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(renderPass->GetColorAttachmentCount(), colorBlendAttachment);

		VkPipelineColorBlendStateCreateInfo colorBlending = VulkanAPI::PipelineColorBlendStateCreateInfo((uint32_t)colorBlendAttachments.size(), colorBlendAttachments.data());

		VkDynamicState dynamicStates[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState = VulkanAPI::PipelineDynamicStateCreateInfo(3, dynamicStates);

		m_CreateInfo.Shader->RTLoadDescriptors(vulkanDevice, m_DescriptorPool);
		const auto& descriptorSetsHandles = m_CreateInfo.Shader->GetDescriptorSetHandles();
		const auto& pushConstants = m_CreateInfo.Shader->GetPushConstants();
		
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
		descriptorSetLayouts.reserve(descriptorSetsHandles.size());
		for (auto handle : descriptorSetsHandles) {
			const auto& descriptorSet = vulkanDevice->AccessResource<VulkanDescriptorSet>(handle);
			descriptorSetLayouts.emplace_back(descriptorSet->GetDescriptorSetLayout());
		}

		std::vector<VkPushConstantRange> pushConstantRanges;
		for (const VulkanPushConstant& pc : pushConstants)
			pushConstantRanges.push_back(pc.GetHandle());

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = VulkanAPI::PipelineLayoutCreateInfo((uint32_t)descriptorSetLayouts.size(), descriptorSetLayouts.data(), (uint32_t)pushConstantRanges.size(), pushConstantRanges.data());

		VkDevice logicalDevice = vulkanDevice->GetLogicalDevice();
		LUCY_VK_ASSERT(vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &m_PipelineLayoutHandle));

		VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = VulkanAPI::PipelineDepthStencilStateCreateInfo(m_CreateInfo.DepthConfiguration.DepthWriteEnable, 
																													  m_CreateInfo.DepthConfiguration.DepthTestEnable,
																													  m_CreateInfo.DepthConfiguration.DepthCompareOp,
																													  VK_FALSE, m_CreateInfo.DepthConfiguration.MinDepth,
																													  m_CreateInfo.DepthConfiguration.MaxDepth,
																													  m_CreateInfo.DepthConfiguration.StencilTestEnable);

		VkGraphicsPipelineCreateInfo pipelineCreateInfo = VulkanAPI::GraphicsPipelineCreateInfo(&vertexInputInfo, &inputAssemblyInfo, &viewportState, &rasterizationCreateInfo,
																								&multisamplingCreateInfo, &depthStencilCreateInfo, &colorBlending, &dynamicState,
																								m_PipelineLayoutHandle,
																								m_CreateInfo.Shader->As<VulkanGraphicsShader>(),
																								renderPass);
		LUCY_VK_ASSERT(vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_PipelineHandle));
#ifdef LUCY_DEBUG
		LUCY_INFO("Vulkan graphics pipeline '{0}' created successfully!", m_CreateInfo.Shader->GetName());
#endif
	}

	void VulkanGraphicsPipeline::RTBind(void* commandBufferHandle) {
		vkCmdBindPipeline((VkCommandBuffer)commandBufferHandle, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineHandle);
	}

	VkFormat VulkanGraphicsPipeline::GetVulkanTypeFromSize(ShaderMemberType type, uint32_t size) const {
		switch (type) {
			case ShaderMemberType::Int: {
				switch (size) {
					case 1: return VK_FORMAT_R32_SINT;
					case 2: return VK_FORMAT_R32G32_SINT;
					case 3: return VK_FORMAT_R32G32B32_SINT;
					case 4: return VK_FORMAT_R32G32B32A32_SINT;
				};
				break;
			}
			case ShaderMemberType::Float: {
				switch (size) {
					case 1: return VK_FORMAT_R32_SFLOAT;
					case 2: return VK_FORMAT_R32G32_SFLOAT;
					case 3: return VK_FORMAT_R32G32B32_SFLOAT;
					case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
				};
				break;
			}
			default:
				LUCY_ASSERT(false);
				return VK_FORMAT_UNDEFINED;
		}
	}

	VkVertexInputBindingDescription VulkanGraphicsPipeline::CreateBindingDescription() const {
		return VulkanAPI::VertexInputBindingDescription(0, CalculateStride(m_CreateInfo.VertexShaderLayout) * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX);
	}

	std::vector<VkVertexInputAttributeDescription> VulkanGraphicsPipeline::CreateAttributeDescription(uint32_t binding) {
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;

		uint32_t offset = 0;

		std::ranges::sort(m_CreateInfo.VertexShaderLayout, {}, &VertexShaderLayoutElement::Location);

		for (const auto& [name, location, type, size] : m_CreateInfo.VertexShaderLayout) {
			VkVertexInputAttributeDescription attributeDescriptor = VulkanAPI::VertexInputAttributeDescription(binding, location, GetVulkanTypeFromSize(type, size), offset);
			offset += size * sizeof(float);

			vertexInputAttributeDescriptions.push_back(attributeDescriptor);
		}
		return vertexInputAttributeDescriptions;
	}

	void VulkanGraphicsPipeline::RTDestroyResource() {
		Renderer::EnqueueToRenderCommandQueue([=](const auto& device) {
			const auto& vulkanDevice = device->As<VulkanRenderDevice>();
			VkDevice logicalDevice = vulkanDevice->GetLogicalDevice();

			m_DescriptorPool->RTDestroyResource();
			m_DescriptorPool = nullptr;
			vkDestroyPipelineLayout(logicalDevice, m_PipelineLayoutHandle, nullptr);
			vkDestroyPipeline(logicalDevice, m_PipelineHandle, nullptr);

			m_PipelineHandle = VK_NULL_HANDLE;
			m_PipelineLayoutHandle = VK_NULL_HANDLE;
		});
	}

	void VulkanGraphicsPipeline::RTRecreate() {
		RTDestroyResource();
		Renderer::EnqueueToRenderCommandQueue([&](const auto& device) {
			const auto& vulkanDevice = device->As<VulkanRenderDevice>();
			Create(vulkanDevice);
		});
	}
}