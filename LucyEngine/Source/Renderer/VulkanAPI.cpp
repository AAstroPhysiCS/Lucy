#include "lypch.h"
#include "VulkanAPI.h"
#include "Context/VulkanSwapChain.h"

#include "Core/Window.h"

namespace Lucy::VulkanAPI {

	VkGraphicsPipelineCreateInfo VulkanAPI::GraphicsPipelineCreateInfo(const VkPipelineVertexInputStateCreateInfo* const vertexInputInfo,
																	   const VkPipelineInputAssemblyStateCreateInfo* const inputAssemblyInfo,
																	   const VkPipelineViewportStateCreateInfo* const viewportState,
																	   const VkPipelineRasterizationStateCreateInfo* const rasterizationCreateInfo,
																	   const VkPipelineMultisampleStateCreateInfo* const multisamplingCreateInfo,
																	   const VkPipelineDepthStencilStateCreateInfo* const depthStencilCreateInfo,
																	   const VkPipelineColorBlendStateCreateInfo* const colorBlending,
																	   const VkPipelineDynamicStateCreateInfo* const dynamicState,
																	   VkPipelineLayout pipelineLayout,
																	   const Ref<VulkanGraphicsShader>& shader, const Ref<VulkanRenderPass>& renderPass) {
		VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.stageCount = 2; //vertex and fragment
		pipelineCreateInfo.pStages = shader->GetShaderStageInfos();

		pipelineCreateInfo.pVertexInputState = vertexInputInfo;
		pipelineCreateInfo.pInputAssemblyState = inputAssemblyInfo;
		pipelineCreateInfo.pViewportState = viewportState;
		pipelineCreateInfo.pRasterizationState = rasterizationCreateInfo;
		pipelineCreateInfo.pMultisampleState = multisamplingCreateInfo;
		pipelineCreateInfo.pDepthStencilState = depthStencilCreateInfo;
		pipelineCreateInfo.pColorBlendState = colorBlending;
		pipelineCreateInfo.pDynamicState = dynamicState;
		pipelineCreateInfo.layout = pipelineLayout;
		pipelineCreateInfo.renderPass = renderPass->GetVulkanHandle();
		pipelineCreateInfo.subpass = 0;

		//not that relevant for now
		pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineCreateInfo.basePipelineIndex = -1;

		return pipelineCreateInfo;
	}

	VkComputePipelineCreateInfo VulkanAPI::ComputePipelineCreateInfo(VkPipelineLayout pipelineLayout, VkPipelineShaderStageCreateInfo stage) {
		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.stage = stage;

		return pipelineInfo;
	}

	VkPipelineLayoutCreateInfo VulkanAPI::PipelineLayoutCreateInfo(uint32_t setLayoutCount, const VkDescriptorSetLayout* const descriptorSetLayouts, uint32_t pushConstantRangeCount, const VkPushConstantRange* const pushConstantRanges) {
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = setLayoutCount;
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts;
		pipelineLayoutInfo.pushConstantRangeCount = pushConstantRangeCount;
		pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges;

		return pipelineLayoutInfo;
	}

	VkPipelineViewportStateCreateInfo VulkanAPI::PipelineViewportStateCreateInfo(uint32_t viewportCount, const VkViewport* const viewports, uint32_t scissorCount, const VkRect2D* const scissors) {
		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = viewportCount;
		viewportState.pViewports = viewports;
		viewportState.scissorCount = scissorCount;
		viewportState.pScissors = scissors;

		return viewportState;
	}

	VkPipelineRasterizationDepthClipStateCreateInfoEXT PipelineRasterizationDepthClipStateCreateInfo(VkBool32 depthClipEnable, VkPipelineRasterizationDepthClipStateCreateFlagsEXT flags) {
		VkPipelineRasterizationDepthClipStateCreateInfoEXT clipState{};
		clipState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT;
		clipState.depthClipEnable = depthClipEnable;
		clipState.flags = flags;
		clipState.pNext = VK_NULL_HANDLE;

		return clipState;
	}

	VkPipelineRasterizationStateCreateInfo VulkanAPI::PipelineRasterizationStateCreateInfo(VkFrontFace frontFace, float lineWidth,
		PolygonMode polygonMode, CullingMode cullingMode, VkBool32 depthClampEnable, VkBool32 rasterizerDiscardEnable,
		VkBool32 depthBiasEnable, float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor,
		const void* pNext) {
		VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo{};
		rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationCreateInfo.depthClampEnable = depthClampEnable;
		rasterizationCreateInfo.rasterizerDiscardEnable = rasterizerDiscardEnable;

		switch (polygonMode) {
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

		switch (cullingMode) {
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

		rasterizationCreateInfo.lineWidth = lineWidth;
		rasterizationCreateInfo.frontFace = frontFace;
		rasterizationCreateInfo.depthBiasEnable = depthBiasEnable;
		rasterizationCreateInfo.depthBiasConstantFactor = depthBiasConstantFactor;
		rasterizationCreateInfo.depthBiasClamp = depthBiasClamp;
		rasterizationCreateInfo.depthBiasSlopeFactor = depthBiasSlopeFactor;
		rasterizationCreateInfo.pNext = pNext;

		return rasterizationCreateInfo;
	}

	VkPipelineMultisampleStateCreateInfo VulkanAPI::PipelineMultisampleStateCreateInfo(VkSampleCountFlagBits rasterizationSamples, VkBool32 alphaToCoverageEnable, VkBool32 alphaToOneEnable,
																					   VkBool32 sampleShadingEnable, float minSampleShading, const VkSampleMask* sampleMask) {
		VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo{};
		multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisamplingCreateInfo.rasterizationSamples = rasterizationSamples;
		multisamplingCreateInfo.alphaToCoverageEnable = alphaToCoverageEnable;
		multisamplingCreateInfo.alphaToOneEnable = alphaToOneEnable;
		multisamplingCreateInfo.sampleShadingEnable = sampleShadingEnable;
		multisamplingCreateInfo.minSampleShading = minSampleShading;
		multisamplingCreateInfo.pSampleMask = sampleMask;

		return multisamplingCreateInfo;
	}

	VkPipelineColorBlendAttachmentState VulkanAPI::PipelineColorBlendAttachmentState(VkBool32 blendEnable, VkBlendFactor srcFac, VkBlendFactor dstFac, VkBlendOp blendOp,
																					 VkBlendFactor alphaSrcFac, VkBlendFactor alphaDstFac, VkBlendOp alphaBlendOp,
																					 VkColorComponentFlags colorWriteMask) {
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.blendEnable = blendEnable;
		colorBlendAttachment.srcColorBlendFactor = srcFac;
		colorBlendAttachment.dstColorBlendFactor = dstFac;
		colorBlendAttachment.colorBlendOp = blendOp;
		colorBlendAttachment.srcAlphaBlendFactor = alphaSrcFac;
		colorBlendAttachment.dstAlphaBlendFactor = alphaDstFac;
		colorBlendAttachment.alphaBlendOp = alphaBlendOp;
		colorBlendAttachment.colorWriteMask = colorWriteMask;

		return colorBlendAttachment;
	}

	VkPipelineColorBlendStateCreateInfo VulkanAPI::PipelineColorBlendStateCreateInfo(uint32_t attachmentCount, const VkPipelineColorBlendAttachmentState* const attachments, VkBool32 logicOpEnable, VkLogicOp logicOp) {
		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = logicOpEnable;
		colorBlending.logicOp = logicOp;
		colorBlending.attachmentCount = attachmentCount;
		colorBlending.pAttachments = attachments;

		return colorBlending;
	}

	VkPipelineDynamicStateCreateInfo VulkanAPI::PipelineDynamicStateCreateInfo(uint32_t dynamicStateCount, const VkDynamicState* const dynamicStates) {
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = dynamicStateCount;
		dynamicState.pDynamicStates = dynamicStates;

		return dynamicState;
	}

	VkPipelineDepthStencilStateCreateInfo VulkanAPI::PipelineDepthStencilStateCreateInfo(VkBool32 depthWriteEnable, VkBool32 depthTestEnable, DepthCompareOp depthCompareOp,
																						 VkBool32 depthBoundsTestEnable, float minDepthBounds, float maxDepthBounds, VkBool32 stencilTestEnable) {
		VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo{};
		depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilCreateInfo.depthWriteEnable = depthWriteEnable;
		depthStencilCreateInfo.depthTestEnable = depthTestEnable;
		depthStencilCreateInfo.depthCompareOp = (VkCompareOp)depthCompareOp;
		depthStencilCreateInfo.depthBoundsTestEnable = depthBoundsTestEnable;
		depthStencilCreateInfo.minDepthBounds = minDepthBounds;
		depthStencilCreateInfo.maxDepthBounds = maxDepthBounds;
		depthStencilCreateInfo.stencilTestEnable = stencilTestEnable;

		return depthStencilCreateInfo;
	}

	VkPipelineVertexInputStateCreateInfo VulkanAPI::PipelineVertexInputStateCreateInfo(uint32_t attributeDescriptorCount, const VkVertexInputAttributeDescription* const attributeDescriptors, uint32_t bindingDescriptorCount, const VkVertexInputBindingDescription* const bindingDescriptors) {
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptorCount;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptors;
		vertexInputInfo.vertexBindingDescriptionCount = bindingDescriptorCount;
		vertexInputInfo.pVertexBindingDescriptions = bindingDescriptors;

		return vertexInputInfo;
	}

	VkPipelineInputAssemblyStateCreateInfo VulkanAPI::PipelineInputAssemblyStateCreateInfo(Topology topology) {
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
		inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		switch (topology) {
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

		return inputAssemblyInfo;
	}

	VkVertexInputBindingDescription VulkanAPI::VertexInputBindingDescription(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate) {
		VkVertexInputBindingDescription vertexInputBindingDescription{};
		vertexInputBindingDescription.binding = binding;
		vertexInputBindingDescription.stride = stride;
		vertexInputBindingDescription.inputRate = inputRate;

		return vertexInputBindingDescription;
	}

	VkVertexInputAttributeDescription VulkanAPI::VertexInputAttributeDescription(uint32_t binding, uint32_t location, VkFormat format, uint32_t offset) {
		VkVertexInputAttributeDescription attributeDescriptor{};
		attributeDescriptor.binding = binding;
		attributeDescriptor.location = location;
		attributeDescriptor.format = format;
		attributeDescriptor.offset = offset;

		return attributeDescriptor;
	}

	VkSwapchainCreateInfoKHR VulkanAPI::SwapchainCreateInfo(const void* swapChainInstance, uint32_t imageCount, VkSwapchainKHR oldSwapChain, VkSurfaceTransformFlagBitsKHR preTransform) {
		const auto* swapChain = (const VulkanSwapChain*)swapChainInstance;

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = swapChain->m_Window->GetVulkanSurface();
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = swapChain->m_SelectedFormat.format;
		createInfo.imageColorSpace = swapChain->m_SelectedFormat.colorSpace;
		createInfo.imageExtent = swapChain->m_SelectedSwapExtent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
		createInfo.preTransform = preTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = swapChain->m_SelectedPresentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = oldSwapChain;

		return createInfo;
	}

	VkPresentInfoKHR VulkanAPI::PresentInfoKHR(uint32_t swapChainCount, const VkSwapchainKHR* const swapChains, const uint32_t* imageIndices, uint32_t waitSemaphoreCount, const VkSemaphore* const semaphores) {
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = waitSemaphoreCount;
		presentInfo.pWaitSemaphores = semaphores;

		presentInfo.swapchainCount = swapChainCount;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = imageIndices;
		presentInfo.pResults = nullptr;

		return presentInfo;
	}

	VkCommandPoolCreateInfo CommandPoolCreateInfo(uint32_t poolFlags, uint32_t queueFamilyIndex) {
		VkCommandPoolCreateInfo createCommandPoolInfo{};
		createCommandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		createCommandPoolInfo.flags = (VkCommandPoolCreateFlags)poolFlags;
		createCommandPoolInfo.queueFamilyIndex = queueFamilyIndex;

		return createCommandPoolInfo;
	}

	VkCommandBufferAllocateInfo CommandBufferAllocateInfo(VkCommandPool commandPool, uint32_t level, uint32_t commandBufferCount) {
		VkCommandBufferAllocateInfo createAllocInfo{};
		createAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		createAllocInfo.commandPool = commandPool;
		createAllocInfo.level = (VkCommandBufferLevel)level;
		createAllocInfo.commandBufferCount = commandBufferCount;

		return createAllocInfo;
	}

	VkCommandBufferBeginInfo VulkanAPI::CommandBufferBeginInfo(uint32_t flags) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = flags;

		return beginInfo;
	}

	VkDescriptorPoolCreateInfo VulkanAPI::DescriptorPoolCreateInfo(uint32_t poolSizeCount, const VkDescriptorPoolSize* const poolSizes, uint32_t maxSets, VkDescriptorPoolCreateFlags flags) {
		VkDescriptorPoolCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		info.poolSizeCount = poolSizeCount;
		info.pPoolSizes = poolSizes;
		info.maxSets = maxSets;
		info.flags = flags;

		return info;
	}

	VkDescriptorSetAllocateInfo VulkanAPI::DescriptorSetAllocateInfo(uint32_t descriptorSetCount, const VkDescriptorSetLayout* const setLayouts, VkDescriptorPool descriptorPool) {
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.pSetLayouts = setLayouts;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = descriptorSetCount;

		return allocInfo;
	}

	VkDescriptorSetVariableDescriptorCountAllocateInfo VulkanAPI::DescriptorSetVariableDescriptorCountAllocateInfo(uint32_t descriptorSetCount, const uint32_t* const descriptorCounts) {
		VkDescriptorSetVariableDescriptorCountAllocateInfo variableCountAllocInfo{};
		variableCountAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
		variableCountAllocInfo.descriptorSetCount = descriptorSetCount;
		variableCountAllocInfo.pDescriptorCounts = descriptorCounts;

		return variableCountAllocInfo;
	}

	VkDescriptorBufferInfo VulkanAPI::DescriptorBufferInfo(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = buffer;
		bufferInfo.offset = offset;
		bufferInfo.range = range;

		return bufferInfo;
	}

	VkWriteDescriptorSet VulkanAPI::WriteDescriptorSet(VkDescriptorSet dstSet, uint32_t dstArrayElement, uint32_t dstBinding, uint32_t descriptorCount, VkDescriptorType type,
													   const VkDescriptorBufferInfo* const bufferInfo, const VkDescriptorImageInfo* const imageInfo, const VkBufferView* const texelBufferView) {
		VkWriteDescriptorSet setWrite{};
		setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		setWrite.dstSet = dstSet;
		setWrite.dstArrayElement = dstArrayElement;
		setWrite.dstBinding = dstBinding;
		setWrite.pTexelBufferView = texelBufferView;

		setWrite.pBufferInfo = bufferInfo;
		setWrite.pImageInfo = imageInfo;

		setWrite.descriptorType = type;
		setWrite.descriptorCount = descriptorCount;

		return setWrite;
	}

	VkDescriptorSetLayoutCreateInfo VulkanAPI::DescriptorSetCreateInfo(uint32_t bindingCount, const VkDescriptorSetLayoutBinding* const layoutBindings) {
		VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo{};
		descriptorLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorLayoutInfo.bindingCount = bindingCount;
		descriptorLayoutInfo.pBindings = layoutBindings;
		descriptorLayoutInfo.pNext = nullptr;

		return descriptorLayoutInfo;
	}

	VkDescriptorSetLayoutBinding VulkanAPI::DescriptorSetLayoutBinding(uint32_t binding, uint32_t descriptorCount, DescriptorType type, VkShaderStageFlags stageFlag) {
		VkDescriptorSetLayoutBinding setLayoutBinding{};
		setLayoutBinding.binding = binding;
		setLayoutBinding.descriptorCount = descriptorCount;
		setLayoutBinding.descriptorType = (VkDescriptorType)ConvertDescriptorType(type);
		setLayoutBinding.stageFlags = stageFlag;
		setLayoutBinding.pImmutableSamplers = nullptr;

		return setLayoutBinding;
	}

	VkDescriptorSetLayoutBindingFlagsCreateInfo VulkanAPI::DescriptorSetLayoutBindingFlagsCreateInfo(uint32_t bindingCount, const VkDescriptorBindingFlags* const bindlessDescriptorFlags) {
		VkDescriptorSetLayoutBindingFlagsCreateInfo extendedLayoutInfo{};
		extendedLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
		extendedLayoutInfo.bindingCount = (uint32_t)bindingCount;
		extendedLayoutInfo.pBindingFlags = bindlessDescriptorFlags;

		return extendedLayoutInfo;
	}

	VkDescriptorImageInfo VulkanAPI::DescriptorImageInfo(VkImageLayout imageLayout, VkImageView imageView, VkSampler sampler) {
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = imageLayout;
		imageInfo.imageView = imageView;
		imageInfo.sampler = sampler;

		return imageInfo;
	}

	VkBufferCreateInfo VulkanAPI::BufferCreateInfo(VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharingMode, uint32_t queueFamilyIndexCount, const uint32_t* const queueFamilyIndices, VkBufferCreateFlags flags) {
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = sharingMode;
		bufferInfo.queueFamilyIndexCount = queueFamilyIndexCount;
		bufferInfo.pQueueFamilyIndices = queueFamilyIndices;
		bufferInfo.flags = flags;
		bufferInfo.pNext = nullptr;

		return bufferInfo;
	}

	VkBufferCopy VulkanAPI::BufferCopy(VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size) {
		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = srcOffset;
		copyRegion.dstOffset = dstOffset;
		copyRegion.size = size;

		return copyRegion;
	}

	VkBufferImageCopy VulkanAPI::BufferImageCopy(VkDeviceSize bufferOffset, uint32_t bufferRowLength, uint32_t bufferImageHeight, VkImageSubresourceLayers imageSubresource, VkOffset3D imageOffset, VkExtent3D imageExtent) {
		VkBufferImageCopy region{};
		region.bufferOffset = bufferOffset;
		region.bufferRowLength = bufferRowLength;
		region.bufferImageHeight = bufferImageHeight;
		region.imageSubresource = imageSubresource;
		region.imageOffset = imageOffset;
		region.imageExtent = imageExtent;

		return region;
	}

	VkImageCreateInfo VulkanAPI::ImageCreateInfo(VkImageType imageType, VkExtent3D extent, uint32_t mipLevels, uint32_t arrayLayers,
												 VkFormat format, VkImageTiling tiling, VkImageLayout initialLayout, VkImageUsageFlags usage,
												 VkSharingMode sharingMode, VkSampleCountFlagBits samples, VkImageCreateFlags flags) {
		VkImageCreateInfo imageCreateInfo{};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = imageType;
		imageCreateInfo.extent = extent;
		imageCreateInfo.mipLevels = mipLevels;
		imageCreateInfo.arrayLayers = arrayLayers;
		imageCreateInfo.format = format;
		imageCreateInfo.tiling = tiling;
		imageCreateInfo.initialLayout = initialLayout;
		imageCreateInfo.usage = usage;
		imageCreateInfo.flags = flags;
		imageCreateInfo.sharingMode = sharingMode;
		imageCreateInfo.samples = samples;

		return imageCreateInfo;
	}

	VkImageViewCreateInfo VulkanAPI::ImageViewCreateInfo(VkImage image, VkImageViewType viewType, VkFormat format, VkImageSubresourceRange subresourceRange, VkComponentMapping components) {
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = image;
		createInfo.viewType = viewType;
		createInfo.format = format;
		createInfo.subresourceRange = subresourceRange;
		createInfo.components = components;

		return createInfo;
	}

	VkImageBlit VulkanAPI::ImageBlit(VkImageSubresourceLayers srcSubresource, VkOffset3D srcOffsets[2], VkImageSubresourceLayers dstSubresource, VkOffset3D dstOffsets[2]) {
		VkImageBlit blit{};
		blit.srcSubresource = srcSubresource;
		std::copy(srcOffsets, srcOffsets + 2, blit.srcOffsets);
		blit.dstSubresource = dstSubresource;
		std::copy(dstOffsets, dstOffsets + 2, blit.dstOffsets);

		return blit;
	}

	VkImageSubresourceLayers VulkanAPI::ImageSubresourceLayers(VkImageAspectFlags aspectMask, uint32_t mipLevel, uint32_t baseArrayLayer, uint32_t layerCount) {
		VkImageSubresourceLayers subresourceLayer{};
		subresourceLayer.aspectMask = aspectMask;
		subresourceLayer.mipLevel = mipLevel;
		subresourceLayer.baseArrayLayer = baseArrayLayer;
		subresourceLayer.layerCount = layerCount;

		return subresourceLayer;
	}

	VkImageSubresourceRange VulkanAPI::ImageSubresourceRange(VkImageAspectFlags aspectMask, uint32_t baseMipLevel, uint32_t baseArrayLayer, uint32_t levelCount, uint32_t layerCount) {
		VkImageSubresourceRange subresourceRange{};
		subresourceRange.aspectMask = aspectMask;
		subresourceRange.baseMipLevel = baseMipLevel;
		subresourceRange.baseArrayLayer = baseArrayLayer;
		subresourceRange.levelCount = levelCount;
		subresourceRange.layerCount = layerCount;

		return subresourceRange;
	}

	VkSamplerCreateInfo VulkanAPI::SamplerCreateInfo(VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressModeU, VkSamplerAddressMode addressModeV, VkSamplerAddressMode addressModeW,
													 VkBool32 anisotropyEnable, float maxAnisotropy, VkBorderColor borderColor, VkBool32 unnormalizedCoordinates, VkBool32 compareEnable,
													 VkCompareOp compareOp, VkSamplerMipmapMode mipmapMode, float mipLodBias, float minLod, float maxLod) {
		VkSamplerCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		createInfo.magFilter = magFilter;
		createInfo.minFilter = minFilter;
		createInfo.addressModeU = addressModeU;
		createInfo.addressModeV = addressModeV;
		createInfo.addressModeW = addressModeW;
		createInfo.anisotropyEnable = anisotropyEnable;
		createInfo.maxAnisotropy = maxAnisotropy;
		createInfo.borderColor = borderColor;
		createInfo.unnormalizedCoordinates = unnormalizedCoordinates;
		//TODO: for pcf
		createInfo.compareEnable = compareEnable;
		createInfo.compareOp = compareOp;
		createInfo.mipmapMode = mipmapMode;
		createInfo.mipLodBias = mipLodBias;
		createInfo.minLod = minLod;
		createInfo.maxLod = maxLod;

		return createInfo;
	}

	VkFramebufferCreateInfo VulkanAPI::FramebufferCreateInfo(VkRenderPass renderPass, uint32_t attachmentCount, const VkImageView* const attachments, uint32_t width, uint32_t height, uint32_t layers) {
		VkFramebufferCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass = renderPass;
		createInfo.attachmentCount = attachmentCount;
		createInfo.pAttachments = attachments;
		createInfo.width = width;
		createInfo.height = height;
		createInfo.layers = layers;

		return createInfo;
	}

	VkShaderModuleCreateInfo VulkanAPI::ShaderModuleCreateInfo(size_t codeSize, const uint32_t* const code) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = codeSize;
		createInfo.pCode = code;

		return createInfo;
	}

	VkPipelineShaderStageCreateInfo VulkanAPI::PipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule module, const char* name, const VkSpecializationInfo* const specializationInfo, VkPipelineShaderStageCreateFlags flags) {
		VkPipelineShaderStageCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		createInfo.stage = stage;
		createInfo.module = module;
		createInfo.pName = name;
		createInfo.pSpecializationInfo = specializationInfo;
		createInfo.flags = flags;

		return createInfo;
	}

	VkSemaphoreCreateInfo VulkanAPI::SemaphoreCreateInfo() {
		VkSemaphoreCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		return createInfo;
	}

	VkFenceCreateInfo VulkanAPI::FenceCreateInfo(VkFenceCreateFlags flags) {
		VkFenceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		createInfo.flags = flags;

		return createInfo;
	}

	VkImageMemoryBarrier VulkanAPI::ImageMemoryBarrier(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subresourceRange,
													   VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex) {
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = image;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.subresourceRange = subresourceRange;
		barrier.srcAccessMask = srcAccessMask;
		barrier.dstAccessMask = dstAccessMask;
		barrier.srcQueueFamilyIndex = srcQueueFamilyIndex; //TODO: Investigate and support queue ownership transfer
		barrier.dstQueueFamilyIndex = dstQueueFamilyIndex;

		return barrier;
	}

	VkRenderPassCreateInfo VulkanAPI::RenderPassCreateInfo(uint32_t attachmentCount, const VkAttachmentDescription* const attachments,
														   uint32_t subpassCount, const VkSubpassDescription* const subpasses,
														   uint32_t dependencyCount, const VkSubpassDependency* const dependencies) {
		VkRenderPassCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		createInfo.attachmentCount = attachmentCount;
		createInfo.pAttachments = attachments;
		createInfo.subpassCount = subpassCount;
		createInfo.pSubpasses = subpasses;
		createInfo.dependencyCount = dependencyCount;
		createInfo.pDependencies = dependencies;

		return createInfo;
	}

	VkRenderPassBeginInfo VulkanAPI::RenderPassBeginInfo(VkRenderPass renderPass, VkFramebuffer frameBuffer, VkRect2D renderArea, uint32_t clearValueCount, const VkClearValue* const clearValues) {
		VkRenderPassBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		beginInfo.renderPass = renderPass;
		beginInfo.framebuffer = frameBuffer;
		beginInfo.renderArea = renderArea;
		beginInfo.clearValueCount = clearValueCount;
		beginInfo.pClearValues = clearValues;

		return beginInfo;
	}

	VkRenderPassMultiviewCreateInfo VulkanAPI::RenderPassMultiviewCreateInfo(uint32_t subpassCount, const uint32_t* const viewMasks, uint32_t correlationMaskCount, const uint32_t* const correlationMasks) {
		VkRenderPassMultiviewCreateInfo renderPassMultiview{};
		renderPassMultiview.sType = VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO;
		renderPassMultiview.subpassCount = subpassCount;
		renderPassMultiview.pViewMasks = viewMasks;
		renderPassMultiview.correlationMaskCount = correlationMaskCount;
		renderPassMultiview.pCorrelationMasks = correlationMasks;

		return renderPassMultiview;
	}

	VkAttachmentDescription VulkanAPI::AttachmentDescription(VkFormat format, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp,
															 VkAttachmentLoadOp stencilLoadOp, VkAttachmentStoreOp stencilStoreOp,
															 VkSampleCountFlagBits samples, VkImageLayout initialLayout, VkImageLayout finalLayout) {
		VkAttachmentDescription attachmentDescription{};
		attachmentDescription.format = format;
		attachmentDescription.loadOp = loadOp;
		attachmentDescription.storeOp = storeOp;
		attachmentDescription.stencilLoadOp = stencilLoadOp;
		attachmentDescription.stencilStoreOp = stencilStoreOp;
		attachmentDescription.samples = samples;
		attachmentDescription.initialLayout = initialLayout;
		attachmentDescription.finalLayout = finalLayout;

		return attachmentDescription;
	}

	VkAttachmentReference VulkanAPI::AttachmentReference(uint32_t attachment, VkImageLayout layout) {
		VkAttachmentReference attachmentReference{};
		attachmentReference.attachment = attachment;
		attachmentReference.layout = layout;

		return attachmentReference;
	}

	VkSubpassDescription VulkanAPI::SubpassDescription(VkPipelineBindPoint pipelineBindPoint, uint32_t colorAttachmentCounts, const VkAttachmentReference* const colorAttachments, const VkAttachmentReference* const depthAttachment,
													   uint32_t inputAttachmentCount, const VkAttachmentReference* const inputAttachments,
													   uint32_t preserveAttachmentCount, const uint32_t* const preserveAttachments,
													   const VkAttachmentReference* const resolveAttachments) {
		VkSubpassDescription subpassDescription{};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = colorAttachmentCounts;
		subpassDescription.pColorAttachments = colorAttachments;
		subpassDescription.inputAttachmentCount = inputAttachmentCount;
		subpassDescription.pInputAttachments = inputAttachments;
		subpassDescription.preserveAttachmentCount = preserveAttachmentCount;
		subpassDescription.pPreserveAttachments = preserveAttachments;
		subpassDescription.pResolveAttachments = resolveAttachments;
		subpassDescription.pDepthStencilAttachment = depthAttachment;

		return subpassDescription;
	}

	VkSubpassDependency VulkanAPI::SubpassDependency(uint32_t srcSubpass, uint32_t dstSubpass, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
													 VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkDependencyFlags dependencyFlags) {
		VkSubpassDependency subpassDependency{};
		subpassDependency.srcSubpass = srcSubpass;
		subpassDependency.dstSubpass = dstSubpass;
		subpassDependency.srcStageMask = srcStageMask;
		subpassDependency.dstStageMask = dstStageMask;
		subpassDependency.srcAccessMask = srcAccessMask;
		subpassDependency.dstAccessMask = dstAccessMask;
		subpassDependency.dependencyFlags = dependencyFlags;

		return subpassDependency;
	}

	VkQueryPoolCreateInfo VulkanAPI::QueryPoolCreateInfo(uint32_t queryCount, VkQueryType queryType, VkQueryPipelineStatisticFlags pipelineStatistics) {
		VkQueryPoolCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		createInfo.queryCount = queryCount;
		createInfo.queryType = queryType;
		createInfo.pipelineStatistics = pipelineStatistics; //pipelineStatistics is ignored if queryType is not VK_QUERY_TYPE_PIPELINE_STATISTICS.
		return createInfo;
	}

	VkSubmitInfo VulkanAPI::QueueSubmitInfo(uint32_t commandBufferCount, const VkCommandBuffer* const commandBuffers,
											uint32_t waitSemaphoreCount, const VkSemaphore* const waitSemaphores, VkPipelineStageFlags* waitStages,
											uint32_t signalSemaphoreCount, const VkSemaphore* const signalSemaphores) {
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		submitInfo.waitSemaphoreCount = waitSemaphoreCount;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = commandBufferCount;
		submitInfo.pCommandBuffers = commandBuffers;
		submitInfo.signalSemaphoreCount = signalSemaphoreCount;
		submitInfo.pSignalSemaphores = signalSemaphores;

		return submitInfo;
	}
}