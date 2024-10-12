#pragma once

#include <vector>

namespace Lucy {

	struct VulkanUniformImageSampler {
		VulkanUniformImageSampler(uint32_t binding, const std::string& name, DescriptorType descriptorType)
			: Binding(binding), Name(name), DescriptorType(descriptorType) {}

		uint32_t Binding = 0;
		std::string Name = "Undefined";
		std::vector<VkDescriptorImageInfo> ImageInfos;
		DescriptorType DescriptorType;
	};
}