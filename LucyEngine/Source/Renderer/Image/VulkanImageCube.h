#pragma once

#include "VulkanImage.h"

namespace Lucy {

	class VulkanImageCube : public VulkanImage {
	public:
		VulkanImageCube(const std::filesystem::path& path, const ImageCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device, std::string_view debugName);
		VulkanImageCube(const ImageCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device, std::string_view debugName);
		virtual ~VulkanImageCube() = default;

		void RTRecreate(uint32_t width, uint32_t height) final override;
	private:
		void RTCreateFromPath();
		void RTCreateEmptyImage();
		void RTDestroyResource() final override;

		Ref<VulkanRenderDevice> m_VulkanDevice = nullptr;
	};
}