#pragma once

#include "vulkan/vulkan.h"

namespace Lucy {

	enum class DescriptorType {
		Undefined,
		SampledImage,
		Sampler,
		CombinedImageSampler,
		StorageImage,
		Buffer,
		DynamicBuffer,
		SSBO,
		SSBODynamic
	};

	DescriptorType ConvertDescriptorType(uint32_t type);
	uint32_t ConvertDescriptorType(DescriptorType type);
}

