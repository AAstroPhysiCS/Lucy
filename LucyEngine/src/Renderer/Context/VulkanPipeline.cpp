#include "lypch.h"
#include "VulkanPipeline.h"

#include "vulkan/vulkan.h"
#include "../Shader/VulkanShader.h"

#include "../VulkanRenderPass.h"

#include "Renderer/Renderer.h"
#include "Renderer/VulkanDescriptors.h"

#include "Renderer/Memory/Buffer/Vulkan/VulkanUniformBuffer.h"
#include "Renderer/Memory/Buffer/Vulkan/VulkanFrameBuffer.h"

namespace Lucy {

	VulkanPipeline::VulkanPipeline(const PipelineCreateInfo& createInfo)
		: Pipeline(createInfo) {
		Renderer::Enqueue([&]() {
			Create();
		});
	}

	void VulkanPipeline::Create() {
		if (!m_DescriptorPool) {
			const std::vector<VkDescriptorPoolSize>& poolSizes = CreateDescriptorPoolSizes();

			VulkanDescriptorPoolCreateInfo poolCreateInfo{};
			poolCreateInfo.PoolSizesVector = poolSizes;
			poolCreateInfo.MaxSet = 100;
			poolCreateInfo.PoolFlags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
			m_DescriptorPool = Memory::CreateRef<VulkanDescriptorPool>(poolCreateInfo);
		}

		const auto& bindingDescriptor = CreateBindingDescription();
		const auto& attributeDescriptor = CreateAttributeDescription(bindingDescriptor.binding);

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptor.size();
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptor.data();
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescriptor;

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
		inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		switch (m_CreateInfo.Topology) {
			case Topology::TRIANGLES:
				inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
				break;
			case Topology::POINTS:
				inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
				break;
			case Topology::LINES:
				inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
				break;
		}
		inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = nullptr;
		viewportState.scissorCount = 1;
		viewportState.pScissors = nullptr;

		VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo{};
		rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationCreateInfo.depthClampEnable = VK_FALSE;
		rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;

		switch (m_CreateInfo.Rasterization.PolygonMode) {
			case PolygonMode::FILL:
				rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
				break;
			case PolygonMode::LINE:
				rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_LINE;
				break;
			case PolygonMode::POINT:
				rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_POINT;
				break;
		}
		rasterizationCreateInfo.lineWidth = m_CreateInfo.Rasterization.LineWidth;
		rasterizationCreateInfo.cullMode = m_CreateInfo.Rasterization.CullingMode;
		rasterizationCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationCreateInfo.depthBiasEnable = VK_FALSE;
		rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
		rasterizationCreateInfo.depthBiasClamp = 0.0f;
		rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f;

		VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo{};
		multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;
		multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisamplingCreateInfo.minSampleShading = 1.0f;
		multisamplingCreateInfo.pSampleMask = nullptr;
		multisamplingCreateInfo.alphaToCoverageEnable = VK_FALSE;
		multisamplingCreateInfo.alphaToOneEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;

		VkDynamicState dynamicStates[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = 3;
		dynamicState.pDynamicStates = dynamicStates;

		ParseBuffers();

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = m_DescriptorSetLayouts.size();
		pipelineLayoutInfo.pSetLayouts = m_DescriptorSetLayouts.data();

		std::vector<VkPushConstantRange> pushConstantRanges;
		for (PushConstant& pc : m_PushConstants)
			pushConstantRanges.push_back(pc.GetHandle());

		pipelineLayoutInfo.pushConstantRangeCount = pushConstantRanges.size();
		pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

		VkDevice device = VulkanDevice::Get().GetLogicalDevice();
		LUCY_VK_ASSERT(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_PipelineLayoutHandle));

		VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo{};
		depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
		depthStencilCreateInfo.depthTestEnable = VK_TRUE;
		depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
		depthStencilCreateInfo.minDepthBounds = 0.0f;
		depthStencilCreateInfo.maxDepthBounds = 1.0f;
		depthStencilCreateInfo.stencilTestEnable = VK_FALSE;

		VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.stageCount = 2; //vertex and fragment
		pipelineCreateInfo.pStages = m_CreateInfo.Shader.As<VulkanShader>()->GetShaderStageInfos();

		pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyInfo;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;
		pipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
		pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
		pipelineCreateInfo.pColorBlendState = &colorBlending;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.layout = m_PipelineLayoutHandle;
		pipelineCreateInfo.renderPass = m_CreateInfo.RenderPass.As<VulkanRenderPass>()->GetVulkanHandle();
		pipelineCreateInfo.subpass = 0;

		//not that relevant for now
		pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineCreateInfo.basePipelineIndex = -1;

		LUCY_VK_ASSERT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_PipelineHandle));
		LUCY_INFO("Vulkan pipeline created successfully!");
	}

	void VulkanPipeline::Bind(PipelineBindInfo bindInfo) {
		vkCmdBindPipeline(bindInfo.CommandBuffer, bindInfo.PipelineBindPoint, m_PipelineHandle);
	}

	void VulkanPipeline::ParseBuffers() {
		if (m_DescriptorSetLayouts.size() != 0) //if the application has been resized
			return;

		VkDevice device = VulkanDevice::Get().GetLogicalDevice();

		for (auto& [set, info] : m_CreateInfo.Shader->GetShaderUniformBlockMap()) {
			std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
			std::vector<VkDescriptorBindingFlags> bindingFlags;

			bool setIsDynamicallyAllocated = false;

			for (auto& buffer : info) {
				VkDescriptorSetLayoutBinding binding{};
				binding.binding = buffer.Binding;
				binding.descriptorCount = buffer.ArraySize == 0 ? 1 : buffer.ArraySize;
				if (buffer.DynamicallyAllocated) {
					buffer.ArraySize = MAX_DYNAMICALLY_ALLOCATED_BUFFER;
					binding.descriptorCount = buffer.ArraySize;
					//ensures that we will fill the array later on, since the binding dynamic
					bindingFlags.push_back(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT);
					setIsDynamicallyAllocated = true;
				} else {
					bindingFlags.push_back(0);
				}
				binding.descriptorType = buffer.Type;
				binding.stageFlags = buffer.StageFlag;
				binding.pImmutableSamplers = nullptr;

				layoutBindings.push_back(binding);
			}

			VkDescriptorSetLayoutBindingFlagsCreateInfo extendedLayoutInfo{};
			extendedLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
			extendedLayoutInfo.bindingCount = layoutBindings.size();
			extendedLayoutInfo.pBindingFlags = bindingFlags.data();

			VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo{};
			descriptorLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptorLayoutInfo.bindingCount = layoutBindings.size();
			descriptorLayoutInfo.pBindings = layoutBindings.data();
			descriptorLayoutInfo.pNext = &extendedLayoutInfo;

			VkDescriptorSetLayout descriptorSetLayout;
			LUCY_VK_ASSERT(vkCreateDescriptorSetLayout(device, &descriptorLayoutInfo, nullptr, &descriptorSetLayout));
			m_DescriptorSetLayouts.push_back(descriptorSetLayout);

			VulkanDescriptorSetCreateInfo setCreateInfo;
			setCreateInfo.Layout = descriptorSetLayout;
			setCreateInfo.Pool = m_DescriptorPool;
			setCreateInfo.SetIndex = set;
			setCreateInfo.SizeOfVariableCounting = setIsDynamicallyAllocated ? MAX_DYNAMICALLY_ALLOCATED_BUFFER : 0;

			Ref<VulkanDescriptorSet> descriptorSet = Memory::CreateRef<VulkanDescriptorSet>(setCreateInfo);

			for (const auto& buffer : info) {
				UniformBufferCreateInfo createInfo;
				createInfo.Binding = buffer.Binding;
				createInfo.Name = buffer.Name;
				createInfo.BufferSize = buffer.BufferSize;

				Ref<VulkanRHIUniformCreateInfo> internalInfo = Memory::CreateRef<VulkanRHIUniformCreateInfo>();
				internalInfo->DescriptorSet = descriptorSet;
				internalInfo->Type = buffer.Type;

				createInfo.InternalInfo = internalInfo;
				createInfo.ShaderMemberVariables = buffer.Members;
				createInfo.ArraySize = buffer.ArraySize;

				Ref<VulkanUniformBuffer> uniformBuffer = UniformBuffer::Create(createInfo).As<VulkanUniformBuffer>();
				m_UniformBuffers.push_back(uniformBuffer);
			}

			m_IndividualSets.push_back(descriptorSet);
		}

		auto& pushConstantMap = m_CreateInfo.Shader->GetPushConstants();
		for (auto& pc : pushConstantMap)
			m_PushConstants.push_back(PushConstant(pc.Name, pc.BufferSize, 0, pc.StageFlag));
	}

	VkFormat VulkanPipeline::GetVulkanTypeFromSize(ShaderDataSize size) {
		switch (size) {
			case ShaderDataSize::Int1: return VK_FORMAT_R32_SINT; break;
			case ShaderDataSize::Float1: return VK_FORMAT_R32_SFLOAT; break;
			case ShaderDataSize::Int2: return VK_FORMAT_R32G32_SINT; break;
			case ShaderDataSize::Float2: return VK_FORMAT_R32G32_SFLOAT; break;
			case ShaderDataSize::Int3: return VK_FORMAT_R32G32B32_SINT; break;
			case ShaderDataSize::Float3: return VK_FORMAT_R32G32B32_SFLOAT; break;
			case ShaderDataSize::Int4: return VK_FORMAT_R32G32B32A32_SINT; break;
			case ShaderDataSize::Float4: return VK_FORMAT_R32G32B32A32_SFLOAT; break;
			case ShaderDataSize::Mat4:	 LUCY_ASSERT(false); break; //in vulkan, that's kinda hard...
		}
	}

	std::vector<VkDescriptorPoolSize> VulkanPipeline::CreateDescriptorPoolSizes() {
		std::vector<VkDescriptorPoolSize> poolSizes;
		poolSizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024 });
		poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024 });
		return poolSizes;
	}

	VkVertexInputBindingDescription VulkanPipeline::CreateBindingDescription() {
		VkVertexInputBindingDescription vertexInputBindingDescription{};
		vertexInputBindingDescription.binding = 0;
		vertexInputBindingDescription.stride = CalculateStride(m_CreateInfo.VertexShaderLayout) * sizeof(float);
		vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return vertexInputBindingDescription;
	}

	std::vector<VkVertexInputAttributeDescription> VulkanPipeline::CreateAttributeDescription(uint32_t binding) {
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;

		uint32_t bufferIndex = 0;
		uint32_t offset = 0;

		for (auto [name, size] : m_CreateInfo.VertexShaderLayout.ElementList) {
			VkVertexInputAttributeDescription attributeDescriptor;
			attributeDescriptor.binding = binding;
			attributeDescriptor.location = bufferIndex++;
			attributeDescriptor.format = GetVulkanTypeFromSize(size);
			attributeDescriptor.offset = offset;
			offset += GetSizeFromType(size) * sizeof(float);

			vertexInputAttributeDescriptions.push_back(attributeDescriptor);
		}
		return vertexInputAttributeDescriptions;
	}

	void VulkanPipeline::Destroy() {
		VkDevice device = VulkanDevice::Get().GetLogicalDevice();

		m_CreateInfo.FrameBuffer->Destroy();
		m_CreateInfo.RenderPass.As<VulkanRenderPass>()->Destroy();

		for (const auto& buffer : m_UniformBuffers)
			buffer->DestroyHandle();
		for (auto& descriptorSetLayout : m_DescriptorSetLayouts)
			vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		m_DescriptorPool->Destroy();
		vkDestroyPipelineLayout(device, m_PipelineLayoutHandle, nullptr);
		vkDestroyPipeline(device, m_PipelineHandle, nullptr);
		//m_CreateInfo.Shader->Destroy(); Shader destroying happens in Renderer::Destroy
	}

	void VulkanPipeline::Recreate(uint32_t width, uint32_t height) {
		//I dont need these uncommented lines, since we are dynamically setting the viewport
		//vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
		//vkDestroyPipeline(device, m_Pipeline, nullptr);

		//Create();
		m_CreateInfo.RenderPass.As<VulkanRenderPass>()->Recreate();
		m_CreateInfo.FrameBuffer.As<VulkanFrameBuffer>()->Recreate(width, height, nullptr);
	}
}