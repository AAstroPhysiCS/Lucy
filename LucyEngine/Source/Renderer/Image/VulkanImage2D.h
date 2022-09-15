#pragma once

#include "VulkanImage.h"

namespace Lucy {

	class VulkanImage2D : public VulkanImage {
	public:
		VulkanImage2D(const std::string& path, ImageCreateInfo& createInfo);
		VulkanImage2D(ImageCreateInfo& createInfo);
		VulkanImage2D(const Ref<VulkanImage2D>& other);
		virtual ~VulkanImage2D() = default;

		void Destroy() final override;
		void Recreate(uint32_t width, uint32_t height);
	private:
		void CreateFromPath();
		void CreateEmptyImage();
		void CreateDepthImage();
	};
}

