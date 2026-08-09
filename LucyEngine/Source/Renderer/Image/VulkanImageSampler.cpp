#include "lypch.h"
#include "VulkanImageSampler.h"

#include "Renderer/Device/VulkanRenderDevice.h"

namespace Lucy {

	VulkanImageSampler::VulkanImageSampler(const ImageSamplerCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device)
		: ImageSampler(createInfo) {
		auto GetImageFilter = [](ImageFilterMode mode) {
			switch (mode) {
				case ImageFilterMode::LINEAR:
					return VK_FILTER_LINEAR;
				case ImageFilterMode::NEAREST:
					return VK_FILTER_NEAREST;
				default:
					return VK_FILTER_MAX_ENUM;
			}
		};

		auto GetImageAddressMode = [](ImageAddressMode mode) {
			switch (mode) {
				case ImageAddressMode::REPEAT:
					return VK_SAMPLER_ADDRESS_MODE_REPEAT;
				case ImageAddressMode::CLAMP_TO_BORDER:
					return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
				case ImageAddressMode::CLAMP_TO_EDGE:
					return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				default:
					return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
			}
		};

		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(device->GetPhysicalDevice(), &properties);

		VkSamplerCreateInfo vkCreateInfo = VulkanAPI::SamplerCreateInfo(GetImageFilter(createInfo.Parameter.Mag), GetImageFilter(createInfo.Parameter.Min),
			GetImageAddressMode(createInfo.Parameter.U), GetImageAddressMode(createInfo.Parameter.V), GetImageAddressMode(createInfo.Parameter.W), VK_TRUE,
			properties.limits.maxSamplerAnisotropy, VK_BORDER_COLOR_INT_OPAQUE_BLACK, VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS, VK_SAMPLER_MIPMAP_MODE_LINEAR,
			0.0f, 0.0f, createInfo.MipmapEnabled ? createInfo.MipmapLevel : 0.0f);

		LUCY_VK_ASSERT(vkCreateSampler(device->GetLogicalDevice(), &vkCreateInfo, nullptr, &m_Handle));
	}

	void VulkanImageSampler::RTDestroyResource(RenderDevice* device) {
		if (!m_Handle)
			return;

		vkDestroySampler(reinterpret_cast<VulkanRenderDevice*>(device)->GetLogicalDevice(), m_Handle, nullptr);
		m_Handle = VK_NULL_HANDLE;
	}
}