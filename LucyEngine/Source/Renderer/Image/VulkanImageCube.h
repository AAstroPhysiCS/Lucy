#pragma once

#include "VulkanImage.h"

namespace Lucy {

	class VulkanImageCube : public VulkanImage {
	public:
		VulkanImageCube(const std::filesystem::path& path, const ImageCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device, std::string_view debugName);
		VulkanImageCube(const ImageCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device, std::string_view debugName);
		virtual ~VulkanImageCube() = default;

		VulkanImageCube(const VulkanImageCube&) = delete;
		VulkanImageCube& operator=(const VulkanImageCube&) = delete;
		VulkanImageCube(VulkanImageCube&&) = delete;
		VulkanImageCube& operator=(VulkanImageCube&&) = delete;

		void RTRecreate(uint32_t width, uint32_t height) final override;
	private:
		void RTCreateFromPath(const Ref<VulkanRenderDevice>& device);
		void RTCreateEmptyImage(const Ref<VulkanRenderDevice>& device);
		void RTDestroyResource(RenderDevice* device) final override;
	};
}