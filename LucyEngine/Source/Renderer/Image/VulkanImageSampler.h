#pragma once

#include "ImageSampler.h"

namespace Lucy {

	class VulkanImageSampler : public ImageSampler {
	public:
		VulkanImageSampler(const ImageSamplerCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device);
		virtual ~VulkanImageSampler() = default;

		void RTDestroyResource() final override;

		inline VkSampler GetVulkanHandle() const { return m_Handle; }
	private:
		VulkanImageSampler() = default;

		VkSampler m_Handle = VK_NULL_HANDLE;
		Ref<VulkanRenderDevice> m_VulkanDevice = nullptr;

		friend class VulkanImage;
		friend class VulkanImage2D;
		friend class VulkanImageCube;
	};
}
