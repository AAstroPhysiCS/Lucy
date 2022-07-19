#pragma once

#include "vulkan/vulkan.h"

namespace Lucy {

	enum class DescriptorType {
		Undefined,
		SampledImage,
		Sampler,
		CombinedImageSampler,
		Buffer,
		DynamicBuffer,
		SSBO,
		SSBODynamic
	};

	extern DescriptorType ConvertVulkanTypeToLucyType(VkDescriptorType type);
	extern VkDescriptorType ConvertLucyTypeToVulkanType(DescriptorType type);
}

