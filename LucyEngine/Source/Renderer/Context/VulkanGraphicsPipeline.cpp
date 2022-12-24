#include "lypch.h"
#include "VulkanGraphicsPipeline.h"

#include "Renderer/Shader/VulkanGraphicsShader.h"

#include "../VulkanRenderPass.h"

#include "Renderer/Renderer.h"
#include "Renderer/Descriptors/VulkanDescriptorSet.h"

#include "Renderer/Memory/Buffer/UniformBuffer.h"
#include "Renderer/Memory/Buffer/SharedStorageBuffer.h"

#include "Renderer/Memory/Buffer/Vulkan/VulkanFrameBuffer.h"

namespace Lucy {

	VulkanGraphicsPipeline::VulkanGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo)
		: GraphicsPipeline(createInfo) {
		Renderer::EnqueueToRenderThread([&]() {
			Create();
		});
	}

	void VulkanGraphicsPipeline::Create() {
		if (!m_DescriptorPool) {
			const std::vector<VkDescriptorPoolSize> poolSizes = {
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 }
			};

			VulkanDescriptorPoolCreateInfo poolCreateInfo;
			poolCreateInfo.PoolSizesVector = poolSizes;
			poolCreateInfo.MaxSet = 100;
			poolCreateInfo.PoolFlags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
			m_DescriptorPool = Memory::CreateRef<VulkanDescriptorPool>(poolCreateInfo);
		}

		const auto& bindingDescriptor = CreateBindingDescription();
		const auto& attributeDescriptor = CreateAttributeDescription(bindingDescriptor.binding);

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = VulkanAPI::PipelineVertexInputStateCreateInfo((uint32_t)attributeDescriptor.size(), attributeDescriptor.data(), 1, &bindingDescriptor);
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = VulkanAPI::PipelineInputAssemblyStateCreateInfo(m_CreateInfo.Topology);
		VkPipelineViewportStateCreateInfo viewportState = VulkanAPI::PipelineViewportStateCreateInfo(1, nullptr, 1, nullptr);
		VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo = VulkanAPI::PipelineRasterizationStateCreateInfo(VK_FRONT_FACE_COUNTER_CLOCKWISE, m_CreateInfo.Rasterization.LineWidth,
																														 m_CreateInfo.Rasterization.PolygonMode, m_CreateInfo.Rasterization.CullingMode);
		VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = VulkanAPI::PipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
		VkPipelineColorBlendAttachmentState colorBlendAttachment = VulkanAPI::PipelineColorBlendAttachmentState(VK_TRUE, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD, 
																												VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD);

		std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(m_CreateInfo.RenderPass.As<VulkanRenderPass>()->GetColorAttachmentCount(), colorBlendAttachment);

		VkPipelineColorBlendStateCreateInfo colorBlending = VulkanAPI::PipelineColorBlendStateCreateInfo((uint32_t)colorBlendAttachments.size(), colorBlendAttachments.data());

		VkDynamicState dynamicStates[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState = VulkanAPI::PipelineDynamicStateCreateInfo(3, dynamicStates);

		VulkanContextPipelineUtils::ParseVulkanDescriptorSets(m_DescriptorSetLayouts, m_CreateInfo.Shader, m_DescriptorPool, m_DescriptorSets, m_PushConstants);

		std::vector<VkPushConstantRange> pushConstantRanges;
		for (VulkanPushConstant& pc : m_PushConstants)
			pushConstantRanges.push_back(pc.GetHandle());

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = VulkanAPI::PipelineLayoutCreateInfo((uint32_t)m_DescriptorSetLayouts.size(), m_DescriptorSetLayouts.data(), (uint32_t)pushConstantRanges.size(), pushConstantRanges.data());

		VkDevice device = VulkanContextDevice::Get().GetLogicalDevice();
		LUCY_VK_ASSERT(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_PipelineLayoutHandle));

		VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = VulkanAPI::PipelineDepthStencilStateCreateInfo(m_CreateInfo.DepthConfiguration.DepthWriteEnable, 
																													  m_CreateInfo.DepthConfiguration.DepthTestEnable, 
																													  m_CreateInfo.DepthConfiguration.DepthCompareOp,
																													  VK_FALSE, m_CreateInfo.DepthConfiguration.MinDepth,
																													  m_CreateInfo.DepthConfiguration.MaxDepth, 
																													  m_CreateInfo.DepthConfiguration.StencilTestEnable);
		VkGraphicsPipelineCreateInfo pipelineCreateInfo = VulkanAPI::GraphicsPipelineCreateInfo(&vertexInputInfo, 
																								&inputAssemblyInfo, 
																								&viewportState,
																								&rasterizationCreateInfo, 
																								&multisamplingCreateInfo,
																								&depthStencilCreateInfo, 
																								&colorBlending, 
																								&dynamicState, 
																								m_PipelineLayoutHandle, 
																								m_CreateInfo.Shader.As<VulkanGraphicsShader>(), 
																								m_CreateInfo.RenderPass.As<VulkanRenderPass>());
		LUCY_VK_ASSERT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_PipelineHandle));
#ifdef LUCY_DEBUG
		LUCY_INFO("Vulkan pipeline created successfully!");
#endif
	}

	void VulkanGraphicsPipeline::Bind(void* commandBufferHandle) {
		vkCmdBindPipeline((VkCommandBuffer)commandBufferHandle, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineHandle);
	}

	VkFormat VulkanGraphicsPipeline::GetVulkanTypeFromSize(ShaderDataSize size) {
		switch (size) {
			case ShaderDataSize::Int1:
				return VK_FORMAT_R32_SINT;
			case ShaderDataSize::Float1:
				return VK_FORMAT_R32_SFLOAT;
			case ShaderDataSize::Int2:
				return VK_FORMAT_R32G32_SINT;
			case ShaderDataSize::Float2:
				return VK_FORMAT_R32G32_SFLOAT;
			case ShaderDataSize::Int3:
				return VK_FORMAT_R32G32B32_SINT;
			case ShaderDataSize::Float3:
				return VK_FORMAT_R32G32B32_SFLOAT;
			case ShaderDataSize::Int4:
				return VK_FORMAT_R32G32B32A32_SINT;
			case ShaderDataSize::Float4:
				return VK_FORMAT_R32G32B32A32_SFLOAT;
			case ShaderDataSize::Mat4:
				LUCY_ASSERT(false); //in vulkan, that's kinda hard...
				return VK_FORMAT_UNDEFINED;
			default:
				LUCY_ASSERT(false);
				return VK_FORMAT_UNDEFINED;
		}
	}

	VkVertexInputBindingDescription VulkanGraphicsPipeline::CreateBindingDescription() {
		return VulkanAPI::VertexInputBindingDescription(0, CalculateStride(m_CreateInfo.VertexShaderLayout) * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX);
	}

	std::vector<VkVertexInputAttributeDescription> VulkanGraphicsPipeline::CreateAttributeDescription(uint32_t binding) {
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;

		uint32_t bufferIndex = 0;
		uint32_t offset = 0;

		for (const auto& [name, size] : m_CreateInfo.VertexShaderLayout) {
			VkVertexInputAttributeDescription attributeDescriptor = VulkanAPI::VertexInputAttributeDescription(binding, bufferIndex++, GetVulkanTypeFromSize(size), offset);
			offset += GetSizeFromType(size) * sizeof(float);

			vertexInputAttributeDescriptions.push_back(attributeDescriptor);
		}
		return vertexInputAttributeDescriptions;
	}

	void VulkanGraphicsPipeline::Destroy() {
		VkDevice device = VulkanContextDevice::Get().GetLogicalDevice();

		m_CreateInfo.FrameBuffer->Destroy();
		m_CreateInfo.RenderPass->Destroy();

		for (const auto& set : m_DescriptorSets)
			set->Destroy();

		for (auto& descriptorSetLayout : m_DescriptorSetLayouts)
			vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		m_DescriptorPool->Destroy();
		vkDestroyPipelineLayout(device, m_PipelineLayoutHandle, nullptr);
		vkDestroyPipeline(device, m_PipelineHandle, nullptr);
		//m_CreateInfo.Shader->Destroy(); Shader destroying happens in Renderer::Destroy
	}

	void VulkanGraphicsPipeline::Recreate(uint32_t width, uint32_t height) {
		//I dont need these uncommented lines, since we are dynamically setting the viewport
		//vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
		//vkDestroyPipeline(device, m_Pipeline, nullptr);

		//Create();
		m_CreateInfo.RenderPass->Recreate();
		m_CreateInfo.FrameBuffer->Recreate(width, height);
	}
}