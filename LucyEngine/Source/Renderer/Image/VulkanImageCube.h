#pragma once

#include "VulkanImage.h"

namespace Lucy {

	class VulkanImageCube : public VulkanImage {
	public:
		VulkanImageCube(const std::filesystem::path& path, const ImageCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device);
		VulkanImageCube(const ImageCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device);
		virtual ~VulkanImageCube() = default;

		void RTRecreate(uint32_t width, uint32_t height);
	private:
		void RTCreateFromPath();
		void RTCreateEmptyImage();
		void RTDestroyResource() final override;

		Ref<VulkanRenderDevice> m_VulkanDevice = nullptr;
	};
}