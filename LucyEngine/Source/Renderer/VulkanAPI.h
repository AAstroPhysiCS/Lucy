#pragma once

#include "vulkan/vulkan.h"

#include "Descriptors/DescriptorType.h"

#include "Pipeline/GraphicsPipeline.h"
#include "Shader/VulkanGraphicsShader.h"
#include "VulkanRenderPass.h"

namespace Lucy::VulkanAPI {

	VkGraphicsPipelineCreateInfo GraphicsPipelineCreateInfo(const VkPipelineVertexInputStateCreateInfo* const vertexInputInfo,
															const VkPipelineInputAssemblyStateCreateInfo* const inputAssemblyInfo,
															const VkPipelineViewportStateCreateInfo* const viewportState,
															const VkPipelineRasterizationStateCreateInfo* const rasterizationCreateInfo,
															const VkPipelineMultisampleStateCreateInfo* const multisamplingCreateInfo,
															const VkPipelineDepthStencilStateCreateInfo* const depthStencilCreateInfo,
															const VkPipelineColorBlendStateCreateInfo* const colorBlending,
															const VkPipelineDynamicStateCreateInfo* const dynamicState,
															VkPipelineLayout pipelineLayout,
															const Ref<VulkanGraphicsShader>& shader, const Ref<VulkanRenderPass>& renderPass);
	VkComputePipelineCreateInfo ComputePipelineCreateInfo(VkPipelineLayout pipelineLayout, VkPipelineShaderStageCreateInfo stage);
	VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo(uint32_t setLayoutCount, const VkDescriptorSetLayout* const descriptorSetLayouts, uint32_t pushConstantRangeCount, const VkPushConstantRange* const pushConstantRanges);
	VkPipelineViewportStateCreateInfo PipelineViewportStateCreateInfo(uint32_t viewportCount, const VkViewport* const viewports, uint32_t scissorCount, const VkRect2D* const scissors);
	VkPipelineRasterizationDepthClipStateCreateInfoEXT PipelineRasterizationDepthClipStateCreateInfo(VkBool32 depthClipEnable, VkPipelineRasterizationDepthClipStateCreateFlagsEXT flags = 0);
	VkPipelineRasterizationStateCreateInfo PipelineRasterizationStateCreateInfo(VkFrontFace frontFace, float lineWidth,
		PolygonMode polygonMode, CullingMode cullingMode,
		VkBool32 depthClampEnable = VK_FALSE, VkBool32 rasterizerDiscardEnable = VK_FALSE,
		VkBool32 depthBiasEnable = VK_FALSE, float depthBiasConstantFactor = 0.0f, 
		float depthBiasClamp = 0.0f, float depthBiasSlopeFactor = 0.0f, const void* pNext = VK_NULL_HANDLE);
	VkPipelineMultisampleStateCreateInfo PipelineMultisampleStateCreateInfo(VkSampleCountFlagBits rasterizationSamples,
																			VkBool32 alphaToCoverageEnable = VK_FALSE, VkBool32 alphaToOneEnable = VK_FALSE,
																			VkBool32 sampleShadingEnable = VK_FALSE, float minSampleShading = 0.0f, const VkSampleMask* sampleMask = nullptr);
	VkPipelineColorBlendAttachmentState PipelineColorBlendAttachmentState(VkBool32 blendEnable, VkBlendFactor srcFac, VkBlendFactor dstFac, VkBlendOp blendOp,
																		  VkBlendFactor alphaSrcFac, VkBlendFactor alphaDstFac, VkBlendOp alphaBlendOp,
																		  VkColorComponentFlags colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);
	VkPipelineColorBlendStateCreateInfo PipelineColorBlendStateCreateInfo(uint32_t attachmentCount, const VkPipelineColorBlendAttachmentState* const attachments, VkBool32 logicOpEnable = VK_FALSE, VkLogicOp logicOp = VK_LOGIC_OP_COPY);
	VkPipelineDynamicStateCreateInfo PipelineDynamicStateCreateInfo(uint32_t dynamicStateCount, const VkDynamicState* const dynamicStates);
	VkPipelineDepthStencilStateCreateInfo PipelineDepthStencilStateCreateInfo(VkBool32 depthWriteEnable, VkBool32 depthTestEnable, DepthCompareOp depthCompareOp,
																			  VkBool32 depthBoundsTestEnable, float minDepthBounds, float maxDepthBounds, VkBool32 stencilTestEnable);
	VkPipelineVertexInputStateCreateInfo PipelineVertexInputStateCreateInfo(uint32_t attributeDescriptorCount, const VkVertexInputAttributeDescription* const attributeDescriptors, 
																			uint32_t bindingDescriptorCount, const VkVertexInputBindingDescription* const bindingDescriptors);
	VkPipelineInputAssemblyStateCreateInfo PipelineInputAssemblyStateCreateInfo(Topology topology);
	VkVertexInputBindingDescription VertexInputBindingDescription(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate);
	VkVertexInputAttributeDescription VertexInputAttributeDescription(uint32_t binding, uint32_t location, VkFormat format, uint32_t offset);

	VkSwapchainCreateInfoKHR SwapchainCreateInfo(const void* swapChainInstance, uint32_t imageCount, VkSwapchainKHR oldSwapChain, VkSurfaceTransformFlagBitsKHR preTransform);
	VkPresentInfoKHR PresentInfoKHR(uint32_t swapChainCount, const VkSwapchainKHR* const swapChains, const uint32_t* imageIndices, uint32_t waitSemaphoreCount, const VkSemaphore* const semaphores);

	VkCommandPoolCreateInfo CommandPoolCreateInfo(uint32_t poolFlags, uint32_t queueFamilyIndex);
	VkCommandBufferAllocateInfo CommandBufferAllocateInfo(VkCommandPool commandPool, uint32_t level, uint32_t commandBufferCount);
	VkCommandBufferBeginInfo CommandBufferBeginInfo(uint32_t flags);

	VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo(uint32_t poolSizeCount, const VkDescriptorPoolSize* const poolSizes, uint32_t maxSets, VkDescriptorPoolCreateFlags flags);
	VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo(uint32_t descriptorSetCount, const VkDescriptorSetLayout* const setLayouts, VkDescriptorPool descriptorPool);
	VkDescriptorSetVariableDescriptorCountAllocateInfo DescriptorSetVariableDescriptorCountAllocateInfo(uint32_t descriptorSetCount, const uint32_t* const descriptorCounts);
	VkDescriptorBufferInfo DescriptorBufferInfo(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);

	VkWriteDescriptorSet WriteDescriptorSet(VkDescriptorSet dstSet, uint32_t dstArrayElement, uint32_t dstBinding, uint32_t descriptorCount, VkDescriptorType type,
											const VkDescriptorBufferInfo* const bufferInfo = nullptr, const VkDescriptorImageInfo* const imageInfo = nullptr, const VkBufferView* const texelBufferView = nullptr);

	VkDescriptorSetLayoutCreateInfo DescriptorSetCreateInfo(uint32_t bindingCount, const VkDescriptorSetLayoutBinding* const layoutBindings);
	VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding(uint32_t binding, uint32_t descriptorCount, DescriptorType type, VkShaderStageFlags stageFlag);
	VkDescriptorSetLayoutBindingFlagsCreateInfo DescriptorSetLayoutBindingFlagsCreateInfo(uint32_t bindingCount, const VkDescriptorBindingFlags* const bindlessDescriptorFlags);

	VkDescriptorImageInfo DescriptorImageInfo(VkImageLayout imageLayout, VkImageView imageView, VkSampler sampler);


	VkBufferCreateInfo BufferCreateInfo(VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharingMode, uint32_t queueFamilyIndexCount = 0, const uint32_t* const queueFamilyIndices = nullptr, VkBufferCreateFlags flags = 0);
	VkBufferCopy BufferCopy(VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size);
	VkBufferImageCopy BufferImageCopy(VkDeviceSize bufferOffset, uint32_t bufferRowLength, uint32_t bufferImageHeight, VkImageSubresourceLayers imageSubresource, VkOffset3D imageOffset, VkExtent3D imageExtent);

	VkImageCreateInfo ImageCreateInfo(VkImageType imageType, VkExtent3D extent, uint32_t mipLevels, uint32_t arrayLayers,
									  VkFormat format, VkImageTiling tiling, VkImageLayout initialLayout, VkImageUsageFlags usage,
									  VkSharingMode sharingMode, VkSampleCountFlagBits samples, VkImageCreateFlags flags);
	VkImageViewCreateInfo ImageViewCreateInfo(VkImage image, VkImageViewType viewType, VkFormat format, VkImageSubresourceRange subresourceRange, VkComponentMapping components);
	VkImageBlit ImageBlit(VkImageSubresourceLayers srcSubresource, VkOffset3D srcOffsets[2], VkImageSubresourceLayers dstSubresource, VkOffset3D dstOffsets[2]);
	VkImageSubresourceLayers ImageSubresourceLayers(VkImageAspectFlags aspectMask, uint32_t mipLevel, uint32_t baseArrayLayer, uint32_t layerCount);
	VkImageSubresourceRange ImageSubresourceRange(VkImageAspectFlags aspectMask, uint32_t baseMipLevel, uint32_t baseArrayLayer, uint32_t levelCount, uint32_t layerCount);

	VkSamplerCreateInfo SamplerCreateInfo(VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressModeU, VkSamplerAddressMode addressModeV, VkSamplerAddressMode addressModeW,
										  VkBool32 anisotropyEnable, float maxAnisotropy, VkBorderColor borderColor, VkBool32 unnormalizedCoordinates, VkBool32 compareEnable,
										  VkCompareOp compareOp, VkSamplerMipmapMode mipmapMode, float mipLodBias, float minLod, float maxLod);

	VkFramebufferCreateInfo FramebufferCreateInfo(VkRenderPass renderPass, uint32_t attachmentCount, const VkImageView* const attachments, uint32_t width, uint32_t height, uint32_t layers);

	VkShaderModuleCreateInfo ShaderModuleCreateInfo(size_t codeSize, const uint32_t* const code);
	VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule module, const char* name, const VkSpecializationInfo* const specializationInfo = nullptr, VkPipelineShaderStageCreateFlags flags = 0);

	VkSemaphoreCreateInfo SemaphoreCreateInfo();
	VkFenceCreateInfo FenceCreateInfo(VkFenceCreateFlags flags);
	VkImageMemoryBarrier ImageMemoryBarrier(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subresourceRange,
											VkAccessFlags srcAccessMask = VK_ACCESS_NONE_KHR, VkAccessFlags dstAccessMask = VK_ACCESS_NONE_KHR,
											uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);

	VkRenderPassCreateInfo RenderPassCreateInfo(uint32_t attachmentCount, const VkAttachmentDescription* const attachments,
												uint32_t subpassCount, const VkSubpassDescription* const subpasses,
												uint32_t dependencyCount, const VkSubpassDependency* const dependencies);
	VkRenderPassBeginInfo RenderPassBeginInfo(VkRenderPass renderPass, VkFramebuffer frameBuffer, VkRect2D renderArea, uint32_t clearValueCount, const VkClearValue* const clearValues);
	VkRenderPassMultiviewCreateInfo RenderPassMultiviewCreateInfo(uint32_t subpassCount, const uint32_t* const viewMasks, uint32_t correlationMaskCount, const uint32_t* const correlationMasks);
	VkAttachmentDescription AttachmentDescription(VkFormat format, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp,
												  VkAttachmentLoadOp stencilLoadOp, VkAttachmentStoreOp stencilStoreOp,
												  VkSampleCountFlagBits samples, VkImageLayout initialLayout, VkImageLayout finalLayout);
	VkAttachmentReference AttachmentReference(uint32_t attachment, VkImageLayout layout);
	VkSubpassDescription SubpassDescription(VkPipelineBindPoint pipelineBindPoint, uint32_t colorAttachmentCounts, const VkAttachmentReference* const colorAttachments, const VkAttachmentReference* const depthAttachment,
											uint32_t inputAttachmentCount = 0, const VkAttachmentReference* const inputAttachments = nullptr, 
											uint32_t preserveAttachmentCount = 0, const uint32_t* const preserveAttachments = nullptr,
											const VkAttachmentReference* const resolveAttachments = nullptr);
	VkSubpassDependency SubpassDependency(uint32_t srcSubpass, uint32_t dstSubpass, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
										  VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkDependencyFlags dependencyFlags);


	VkQueryPoolCreateInfo QueryPoolCreateInfo(uint32_t queryCount, VkQueryType queryType, VkQueryPipelineStatisticFlags pipelineStatistics);
	VkSubmitInfo QueueSubmitInfo(uint32_t commandBufferCount, const VkCommandBuffer* const commandBuffers,
								 uint32_t waitSemaphoreCount, const VkSemaphore* const waitSemaphores, VkPipelineStageFlags* waitDstStageMask,
								 uint32_t signalSemaphoreCount, const VkSemaphore* const signalSemaphores);
}