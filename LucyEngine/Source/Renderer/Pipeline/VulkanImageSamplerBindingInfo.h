#pragma once

#include <vector>

#include "Utilities/GenerationalPool.h"

#include "Renderer/Descriptors/DescriptorType.h"

namespace Lucy {

	struct VulkanImageDescriptor {
		RenderDeviceResourceHandle ImageHandle{};
		uint32_t Mip = INVALID_INDEX;
		VkDescriptorImageInfo ImageInfo{};
	};

	struct VulkanImageSamplerBindingInfo {
		VulkanImageSamplerBindingInfo(uint32_t binding, const std::string& name, DescriptorType descriptorType)
			: Binding(binding), Name(name), DescriptorType(descriptorType) {}

		uint32_t Binding = 0;
		std::string Name = "Undefined";

		GenerationalPool<RenderDeviceTextureHandle, VulkanImageDescriptor> Images;

		DescriptorType DescriptorType;
	};
}