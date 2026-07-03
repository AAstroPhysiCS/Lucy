#include "lypch.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanUniformImageSampler.h"

#include "Renderer/Shader/VulkanGraphicsShader.h"

#include "../VulkanRenderPass.h"

#include "../Context/VulkanContext.h"

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
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_DYNAMIC_DESCRIPTOR_COUNT * 2 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_DYNAMIC_DESCRIPTOR_COUNT * 5 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, MAX_DYNAMIC_DESCRIPTOR_COUNT * 5 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_DYNAMIC_DESCRIPTOR_COUNT * 2 }
			};
#else
			const std::vector<VkDescriptorPoolSize> poolSizes = {
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_DYNAMIC_DESCRIPTOR_COUNT * 2 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_DYNAMIC_DESCRIPTOR_COUNT * 5 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, MAX_DYNAMIC_DESCRIPTOR_COUNT * 5 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_DYNAMIC_DESCRIPTOR_COUNT * 2 }
			};
#endif
			VulkanDescriptorPoolCreateInfo poolCreateInfo;
			poolCreateInfo.PoolSizesVector = poolSizes;
#if USE_INTEGRATED_GRAPHICS
			poolCreateInfo.MaxSet = 10;
#else
			poolCreateInfo.MaxSet = 10;
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
		LUCY_INFO("Vulkan graphics pipeline '{0}' created successfully!", m_CreateInfo.Shader->GetName());
#ifdef LUCY_DEBUG
		std::string objectName = std::format("{0} Graphics Pipeline", m_CreateInfo.Shader->GetName());

		VkDebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = VK_OBJECT_TYPE_PIPELINE;
		nameInfo.objectHandle = reinterpret_cast<uint64_t>(m_PipelineHandle);
		nameInfo.pObjectName = objectName.c_str();

		VulkanExternalFuncLinkage::vkSetDebugUtilsObjectNameEXT(logicalDevice, &nameInfo);
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

	void VulkanGraphicsPipeline::RTLoadDescriptors(const Ref<RenderDevice>& device) {
		const auto& shader = GetShader();

		const auto& reflectPushConstants = shader->GetShaderPushConstants();
		const auto& reflectUniformBlockMaps = shader->GetShaderUniformBlockMap();

		for (const auto& [set, info] : reflectUniformBlockMaps) {
			DescriptorSetCreateInfo createInfo{
				.SetIndex = set,
				.ShaderVariables = info,
			};
			RenderResourceHandle descriptorSetHandle = device->CreateDescriptorSet(createInfo);
			const auto& descriptorSet = device->AccessResource<VulkanDescriptorSet>(descriptorSetHandle);
			descriptorSet->RTBake(m_DescriptorPool);
			AddDescriptorSetHandle(descriptorSetHandle); //maybe just store the handle?
		}

		for (auto& pc : reflectPushConstants)
			AddPushConstant(pc);
	}

	VkVertexInputBindingDescription VulkanGraphicsPipeline::CreateBindingDescription() const {
		return Vertex::GetBindingDescription();
	}

	std::vector<VkVertexInputAttributeDescription> VulkanGraphicsPipeline::CreateAttributeDescription(uint32_t binding) {
		std::vector<VkVertexInputAttributeDescription> result;
		const auto& attributes = Vertex::GetAttributeDescriptions(binding);
		const auto& shaderLayout = GetShader()->GetVertexShaderLayout();

		for (const auto& element : shaderLayout) {
			for (const auto& attribute : attributes) {
				if (element.Location == attribute.location) {
					result.push_back(attribute);
				}
			}
		}

		return result;
	}

	void VulkanGraphicsPipeline::RTDestroyResource() {
		Pipeline::RTDestroyResource();

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