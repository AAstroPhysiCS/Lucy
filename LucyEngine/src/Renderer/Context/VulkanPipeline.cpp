#include "lypch.h"
#include "VulkanPipeline.h"

#include "vulkan/vulkan.h"
#include "../Shader/VulkanShader.h"

#include "../VulkanRenderPass.h"
#include "Renderer/Buffer/Vulkan/VulkanFrameBuffer.h"

#include "Renderer/Renderer.h"
#include "Renderer/VulkanDescriptors.h"
#include "Renderer/Buffer/Vulkan/VulkanUniformBuffer.h"

namespace Lucy {

	VulkanPipeline::VulkanPipeline(const PipelineSpecification& specs)
		: Pipeline(specs) {
		Renderer::Enqueue([&]() {
			Create();
		});
	}

	void VulkanPipeline::Bind(PipelineBindInfo bindInfo) {
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();

		vkCmdBindPipeline(bindInfo.CommandBuffer, bindInfo.PipelineBindPoint, m_PipelineHandle);

		//organizing all "distinct!" sets
		std::vector<VkDescriptorSet> descriptorSetsToBind;
		for (VulkanDescriptorSet descriptorSet : m_IndividualSets) {
			descriptorSetsToBind.push_back(descriptorSet.GetSetBasedOffCurrentFrame(swapChain.GetCurrentFrameIndex()));
		}

		if (descriptorSetsToBind.size() != 0)
			vkCmdBindDescriptorSets(bindInfo.CommandBuffer, bindInfo.PipelineBindPoint, m_PipelineLayoutHandle, 0, descriptorSetsToBind.size(), descriptorSetsToBind.data(), 0, nullptr);

		RenderPassBeginInfo renderPassBeginInfo;
		renderPassBeginInfo.CommandBuffer = bindInfo.CommandBuffer;
		renderPassBeginInfo.VulkanFrameBuffer = As(m_Specs.FrameBuffer, VulkanFrameBuffer)->GetVulkanHandles()[swapChain.GetCurrentImageIndex()];
		m_Specs.RenderPass->Begin(renderPassBeginInfo);
	}

	void VulkanPipeline::Unbind() {
		m_Specs.RenderPass->End();
	}

	void VulkanPipeline::Create() {
		if (!m_DescriptorPool) {
			const std::vector<VkDescriptorPoolSize>& poolSizes = CreateDescriptorPoolSizes();

			VulkanDescriptorPoolSpecifications poolSpecs{};
			poolSpecs.PoolSizesVector = poolSizes;
			poolSpecs.MaxSet = 100;
			m_DescriptorPool = CreateRef<VulkanDescriptorPool>(poolSpecs);
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
		switch (m_Specs.Topology) {
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

		switch (m_Specs.Rasterization.PolygonMode) {
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
		rasterizationCreateInfo.lineWidth = m_Specs.Rasterization.LineWidth;
		rasterizationCreateInfo.cullMode = m_Specs.Rasterization.CullingMode;
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

		ParseUniformBuffers();

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = m_DescriptorSetLayouts.size();
		pipelineLayoutInfo.pSetLayouts = m_DescriptorSetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		VkDevice device = VulkanDevice::Get().GetLogicalDevice();
		LUCY_VK_ASSERT(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_PipelineLayoutHandle));

		VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.stageCount = 2; //vertex and fragment
		pipelineCreateInfo.pStages = As(m_Specs.Shader, VulkanShader)->GetShaderStageInfos();

		pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyInfo;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;
		pipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
		pipelineCreateInfo.pDepthStencilState = nullptr; //TODO
		pipelineCreateInfo.pColorBlendState = &colorBlending;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.layout = m_PipelineLayoutHandle;
		pipelineCreateInfo.renderPass = As(m_Specs.RenderPass, VulkanRenderPass)->GetVulkanHandle();
		pipelineCreateInfo.subpass = 0;

		//not that relevant for now
		pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineCreateInfo.basePipelineIndex = -1;

		LUCY_VK_ASSERT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_PipelineHandle));
		LUCY_INFO("Vulkan pipeline created successfully!");
	}

	void VulkanPipeline::ParseUniformBuffers() {
		if (m_DescriptorSetLayouts.size() != 0) return; //if the application has been resized
		VkDevice device = VulkanDevice::Get().GetLogicalDevice();

		for (auto& [set, info] : m_Specs.Shader->GetDescriptorSetMap()) {
			std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
			uint32_t size = 0;

			for (auto& ub : info) {
				VkDescriptorSetLayoutBinding binding{};
				binding.binding = ub.Binding;
				binding.descriptorCount = 1;
				binding.descriptorType = ub.Type;
				binding.stageFlags = ub.StageFlag;
				binding.pImmutableSamplers = nullptr;
				size += ub.BufferSize;
				layoutBindings.push_back(binding);
			}

			VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo{};
			descriptorLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptorLayoutInfo.bindingCount = layoutBindings.size();
			descriptorLayoutInfo.pBindings = layoutBindings.data();

			VkDescriptorSetLayout descriptorSetLayout;
			LUCY_VK_ASSERT(vkCreateDescriptorSetLayout(device, &descriptorLayoutInfo, nullptr, &descriptorSetLayout));
			m_DescriptorSetLayouts.push_back(descriptorSetLayout);

			VulkanDescriptorSetSpecifications setSpecs;
			setSpecs.Layout = descriptorSetLayout;
			setSpecs.Pool = m_DescriptorPool;

			VulkanDescriptorSet descriptorSet(setSpecs);
			for (auto& ub : info) {
				RefLucy<VulkanUniformBuffer> uniformBuffer = As(UniformBuffer::Create(size, ub.Binding, std::optional<VulkanDescriptorSet>(descriptorSet)), VulkanUniformBuffer);
				m_UniformBuffers.push_back(uniformBuffer);
			}
			m_IndividualSets.push_back(descriptorSet);
		}
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

	//TODO: Understand this... 10 is a temporary number
	std::vector<VkDescriptorPoolSize> VulkanPipeline::CreateDescriptorPoolSizes() {
		std::vector<VkDescriptorPoolSize> poolSizes;
		poolSizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 });
		poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 });
		return poolSizes;
	}

	VkVertexInputBindingDescription VulkanPipeline::CreateBindingDescription() {
		VkVertexInputBindingDescription vertexInputBindingDescription{};
		vertexInputBindingDescription.binding = 0;
		vertexInputBindingDescription.stride = CalculateStride(m_Specs.VertexShaderLayout) * sizeof(float);
		vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return vertexInputBindingDescription;
	}

	std::vector<VkVertexInputAttributeDescription> VulkanPipeline::CreateAttributeDescription(uint32_t binding) {
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;

		uint32_t bufferIndex = 0;
		uint32_t offset = 0;

		for (auto [name, size] : m_Specs.VertexShaderLayout.ElementList) {
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

		m_Specs.FrameBuffer->Destroy();
		As(m_Specs.RenderPass, VulkanRenderPass)->Destroy();

		for (const auto& buffer : m_UniformBuffers)
			buffer->Destroy();
		for (auto& descriptorSetLayout : m_DescriptorSetLayouts)
			vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		m_DescriptorPool->Destroy();
		vkDestroyPipelineLayout(device, m_PipelineLayoutHandle, nullptr);
		vkDestroyPipeline(device, m_PipelineHandle, nullptr);
		//m_Specs.Shader->Destroy(); Shader destroying happens in Renderer::Destroy
	}

	void VulkanPipeline::Recreate() {
		//I dont need these uncommented lines, since we are dynamically setting the viewport
		//vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
		//vkDestroyPipeline(device, m_Pipeline, nullptr);

		//Create();
		Renderer::Enqueue([&]() {
			As(m_Specs.FrameBuffer, VulkanFrameBuffer)->Recreate();
			As(m_Specs.RenderPass, VulkanRenderPass)->Recreate();
		});
	}

}