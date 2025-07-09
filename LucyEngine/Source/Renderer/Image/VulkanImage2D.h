#pragma once

#include "VulkanImage.h"

namespace Lucy {

	class VulkanRenderDevice;

	class VulkanImage2D : public VulkanImage {
	public:
		VulkanImage2D(const std::filesystem::path& path, const ImageCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device);
		VulkanImage2D(const ImageCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device);
		VulkanImage2D(const Ref<VulkanImage2D>& other, const Ref<VulkanRenderDevice>& device);
		virtual ~VulkanImage2D() = default;

		void RTRecreate(uint32_t width, uint32_t height) final override;
	private:
		void RTCreateFromPath();
		void RTCreateEmptyImage();
		void RTCreateDepthImage();

		void RTDestroyResource() final override;

		Ref<VulkanRenderDevice> m_VulkanDevice = nullptr;
	};
}

