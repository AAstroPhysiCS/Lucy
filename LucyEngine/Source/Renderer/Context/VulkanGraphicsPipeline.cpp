#include "lypch.h"
#include "VulkanGraphicsPipeline.h"

#include "../Shader/VulkanGraphicsShader.h"

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

		switch (m_CreateInfo.Rasterization.CullingMode) {
			case CullingMode::None:
				rasterizationCreateInfo.cullMode = VK_CULL_MODE_NONE;
				break;
			case CullingMode::Front:
				rasterizationCreateInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
				break;
			case CullingMode::Back:
				rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
				break;
			case CullingMode::FrontAndBack:
				rasterizationCreateInfo.cullMode = VK_CULL_MODE_FRONT_AND_BACK;
				break;
		}

		rasterizationCreateInfo.lineWidth = m_CreateInfo.Rasterization.LineWidth;
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

		std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(m_CreateInfo.RenderPass.As<VulkanRenderPass>()->GetColorAttachmentCount(), colorBlendAttachment);

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = colorBlendAttachments.size();
		colorBlending.pAttachments = colorBlendAttachments.data();

		VkDynamicState dynamicStates[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = 3;
		dynamicState.pDynamicStates = dynamicStates;

		ParseDescriptorSets();

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = m_DescriptorSetLayouts.size();
		pipelineLayoutInfo.pSetLayouts = m_DescriptorSetLayouts.data();

		std::vector<VkPushConstantRange> pushConstantRanges;
		for (VulkanPushConstant& pc : m_PushConstants)
			pushConstantRanges.push_back(pc.GetHandle());

		pipelineLayoutInfo.pushConstantRangeCount = pushConstantRanges.size();
		pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

		VkDevice device = VulkanContextDevice::Get().GetLogicalDevice();
		LUCY_VK_ASSERT(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_PipelineLayoutHandle));

		VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo{};
		depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilCreateInfo.depthWriteEnable = m_CreateInfo.DepthConfiguration.DepthWriteEnable;
		depthStencilCreateInfo.depthTestEnable = m_CreateInfo.DepthConfiguration.DepthTestEnable;
		depthStencilCreateInfo.depthCompareOp = (VkCompareOp)m_CreateInfo.DepthConfiguration.DepthCompareOp;
		depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
		depthStencilCreateInfo.minDepthBounds = m_CreateInfo.DepthConfiguration.MinDepth;
		depthStencilCreateInfo.maxDepthBounds = m_CreateInfo.DepthConfiguration.MaxDepth;
		depthStencilCreateInfo.stencilTestEnable = m_CreateInfo.DepthConfiguration.StencilTestEnable;

		VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.stageCount = 2; //vertex and fragment
		pipelineCreateInfo.pStages = m_CreateInfo.Shader.As<VulkanGraphicsShader>()->GetShaderStageInfos();

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
#ifdef LUCY_DEBUG
		LUCY_INFO("Vulkan pipeline created successfully!");
#endif
	}

	void VulkanGraphicsPipeline::Bind(const VulkanGraphicsPipelineBindInfo& bindInfo) {
		vkCmdBindPipeline(bindInfo.CommandBuffer, bindInfo.PipelineBindPoint, m_PipelineHandle);
	}

	void VulkanGraphicsPipeline::ParseDescriptorSets() {
		if (m_DescriptorSetLayouts.size() != 0) //if the application has been resized
			return;

		VkDevice device = VulkanContextDevice::Get().GetLogicalDevice();

		for (auto& [set, info] : m_CreateInfo.Shader->GetShaderUniformBlockMap()) {
			std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
			std::vector<bool> isBindlessVector;

			for (auto& buffer : info) {
				VkDescriptorSetLayoutBinding binding{};
				binding.binding = buffer.Binding;
				binding.descriptorCount = buffer.ArraySize == 0 ? 1 : buffer.ArraySize;

				isBindlessVector.push_back(buffer.DynamicallyAllocated); //the set is bindless if true

				if (buffer.DynamicallyAllocated) {
					buffer.ArraySize = MAX_DYNAMIC_DESCRIPTOR_COUNT;
					binding.descriptorCount = buffer.ArraySize;
				}

				binding.descriptorType = (VkDescriptorType)ConvertDescriptorType(buffer.Type);
				binding.stageFlags = buffer.StageFlag;
				binding.pImmutableSamplers = nullptr;

				layoutBindings.push_back(binding);
			}

			VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo{};
			descriptorLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptorLayoutInfo.bindingCount = layoutBindings.size();
			descriptorLayoutInfo.pBindings = layoutBindings.data();
			descriptorLayoutInfo.pNext = nullptr;

			/*
			* cant summarize these since these are stack allocated
			*
			* indicates that this is a variable-sized descriptor binding whose size will be specified when a descriptor set is allocated using this layout.
			* The value of descriptorCount is treated as an upper bound on the size of the binding.
			*/

			std::vector<VkDescriptorBindingFlags> bindlessDescriptorFlags;

			for (uint32_t i = 0; i < isBindlessVector.size(); i++) {
				if (isBindlessVector[i]) {
					bindlessDescriptorFlags.push_back(VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT);
					continue;
				}
				bindlessDescriptorFlags.push_back(0); //yes, we really need the 0.
			}

			VkDescriptorSetLayoutBindingFlagsCreateInfo extendedLayoutInfo{};
			extendedLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
			extendedLayoutInfo.bindingCount = layoutBindings.size();
			extendedLayoutInfo.pBindingFlags = bindlessDescriptorFlags.data();

			descriptorLayoutInfo.pNext = &extendedLayoutInfo;

			VkDescriptorSetLayout descriptorSetLayout;
			LUCY_VK_ASSERT(vkCreateDescriptorSetLayout(device, &descriptorLayoutInfo, nullptr, &descriptorSetLayout));
			m_DescriptorSetLayouts.push_back(descriptorSetLayout);

			DescriptorSetCreateInfo setCreateInfo;
			setCreateInfo.SetIndex = set;

			/*
			* if any of the bindings are bindless.
			* we are assuming that the set is being used ONLY for bindless descriptors
			* which means, we can't mix a binding which DOESN'T use bindless with a binding that USES bindless
			*/
			setCreateInfo.Bindless = std::any_of(isBindlessVector.begin(), isBindlessVector.end(), [](bool out) { return out; });

			auto setInternalInfo = Memory::CreateRef<VulkanDescriptorSetCreateInfo>();
			setInternalInfo->Layout = descriptorSetLayout;
			setInternalInfo->Pool = m_DescriptorPool;

			setCreateInfo.InternalInfo = setInternalInfo;

			Ref<DescriptorSet> descriptorSet = DescriptorSet::Create(setCreateInfo);

			for (const auto& buffer : info) {
				switch (buffer.Type) {
					case DescriptorType::SSBODynamic:
					case DescriptorType::SSBO: {
						SharedStorageBufferCreateInfo createInfo;
						createInfo.Name = buffer.Name;
						createInfo.Binding = buffer.Binding;
						createInfo.Type = buffer.Type;
						createInfo.BufferSize = MAX_DYNAMICALLY_ALLOCATED_BUFFER_SIZE;
						createInfo.ArraySize = buffer.ArraySize;
						createInfo.ShaderMemberVariables = buffer.Members;

						descriptorSet->AddBuffer(SharedStorageBuffer::Create(createInfo));
						break;
					}
					default: { //all the other descriptor types are uniforms
						UniformBufferCreateInfo createInfo;
						createInfo.Name = buffer.Name;
						createInfo.Binding = buffer.Binding;
						createInfo.Type = buffer.Type;
						createInfo.BufferSize = buffer.BufferSize;
						createInfo.ArraySize = buffer.ArraySize;
						createInfo.ShaderMemberVariables = buffer.Members;

						descriptorSet->AddBuffer(UniformBuffer::Create(createInfo));
						break;
					}
				}
			}

			m_DescriptorSets.push_back(descriptorSet);
		}

		auto& pushConstantMap = m_CreateInfo.Shader->GetPushConstants();
		for (auto& pc : pushConstantMap)
			m_PushConstants.push_back(VulkanPushConstant(pc.Name, pc.BufferSize, 0, pc.StageFlag));
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
			default:
				LUCY_ASSERT(false);
		}
	}

	VkVertexInputBindingDescription VulkanGraphicsPipeline::CreateBindingDescription() {
		VkVertexInputBindingDescription vertexInputBindingDescription{};
		vertexInputBindingDescription.binding = 0;
		vertexInputBindingDescription.stride = CalculateStride(m_CreateInfo.VertexShaderLayout) * sizeof(float);
		vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return vertexInputBindingDescription;
	}

	std::vector<VkVertexInputAttributeDescription> VulkanGraphicsPipeline::CreateAttributeDescription(uint32_t binding) {
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;

		uint32_t bufferIndex = 0;
		uint32_t offset = 0;

		for (const auto& [name, size] : m_CreateInfo.VertexShaderLayout) {
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