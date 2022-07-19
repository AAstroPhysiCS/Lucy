#include "lypch.h"
#include "DescriptorType.h"

namespace Lucy {

	DescriptorType ConvertVulkanTypeToLucyType(VkDescriptorType type) {
		switch (type) {
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				return DescriptorType::CombinedImageSampler;
			case VK_DESCRIPTOR_TYPE_SAMPLER:
				return DescriptorType::Sampler;
			case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				return DescriptorType::SampledImage;
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
				return DescriptorType::Buffer;
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
				return DescriptorType::DynamicBuffer;
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
				return DescriptorType::SSBO;
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
				return DescriptorType::SSBODynamic;
			default:
				LUCY_ASSERT(false);
				return DescriptorType::Undefined;
		}
	}

	VkDescriptorType ConvertLucyTypeToVulkanType(DescriptorType type) {
		switch (type) {
			case DescriptorType::CombinedImageSampler:
				return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			case DescriptorType::Sampler:
				return VK_DESCRIPTOR_TYPE_SAMPLER;
			case DescriptorType::SampledImage:
				return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			case DescriptorType::Buffer:
				return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			case DescriptorType::DynamicBuffer:
				return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			case DescriptorType::SSBO:
				return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			case DescriptorType::SSBODynamic:
				return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
			default:
				LUCY_ASSERT(false);
				return VK_DESCRIPTOR_TYPE_MAX_ENUM;
		}
	}
}