#include "lypch.h"
#include "VulkanImageCube.h"

#include "Renderer/Renderer.h"
#include "Renderer/Device/VulkanRenderDevice.h"

#include "stb/stb_image.h"

namespace Lucy {

	VulkanImageCube::VulkanImageCube(const std::filesystem::path& path, const ImageCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device, std::string_view debugName)
		: VulkanImage(path, createInfo, debugName), m_VulkanDevice(device) {
		m_CreateInfo.Layers = 6;

		LUCY_ASSERT(m_CreateInfo.ImageType == ImageType::TypeCube);
		LUCY_ASSERT(stbi_is_hdr(m_Path.generic_string().c_str()), "The texture isnt HDR. Texture path: {0}", m_Path.generic_string().c_str());

		RTCreateFromPath();

#if LUCY_DEBUG
		AddLabel(m_Image, device);
#endif
	}

	VulkanImageCube::VulkanImageCube(const ImageCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device, std::string_view debugName)
		: VulkanImage(createInfo, debugName), m_VulkanDevice(device) {
		m_CreateInfo.Layers = 6;
		LUCY_ASSERT(m_CreateInfo.ImageType == ImageType::TypeCube);
		RTCreateEmptyImage();

#if LUCY_DEBUG
		AddLabel(m_Image, device);
#endif
	}

	void VulkanImageCube::RTCreateFromPath() {
		VkImageUsageFlags flags = GetImageFlagsBasedOnUsage();

		VulkanAllocator& allocator = m_VulkanDevice->GetAllocator();
		allocator.CreateVulkanImageVma(m_CreateInfo.Width, m_CreateInfo.Height, m_MaxMipLevel, (VkFormat)GetAPIImageFormat(m_CreateInfo.Format), m_CurrentLayout,
									   flags, VK_IMAGE_TYPE_2D, m_Image, m_ImageVma, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, m_CreateInfo.Layers);

		CalculateMaxMipLevel();

		if (m_CreateInfo.GenerateMipmap)
			GenerateMipmapsImmediate();
		else //transitioning only then, when we dont care about mipmapping. Mipmapping already transitions to the right layout
			SetLayoutImmediate(GetInitialImageLayout(), 0, 0, 1, m_CreateInfo.Layers);

		RTCreateSampler(m_VulkanDevice);
		RTCreateVulkanImageViewHandle(m_VulkanDevice);
	}

	void VulkanImageCube::RTCreateEmptyImage() {
		VkImageUsageFlags flags = GetImageFlagsBasedOnUsage();

		VulkanAllocator& allocator = m_VulkanDevice->GetAllocator();
		allocator.CreateVulkanImageVma(m_CreateInfo.Width, m_CreateInfo.Height, m_MaxMipLevel, (VkFormat)GetAPIImageFormat(m_CreateInfo.Format), m_CurrentLayout,
			flags, VK_IMAGE_TYPE_2D, m_Image, m_ImageVma, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, m_CreateInfo.Layers);
		
		SetLayoutImmediate(GetInitialImageLayout(), 0, 0, 1, m_CreateInfo.Layers);

		RTCreateSampler(m_VulkanDevice);
		RTCreateVulkanImageViewHandle(m_VulkanDevice);
	}

	void VulkanImageCube::RTRecreate(uint32_t width, uint32_t height) {
		m_CreateInfo.Width = width;
		m_CreateInfo.Height = height;

		m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		RTDestroyResource();

		if (!m_Path.empty()) {
			RTCreateFromPath();
		} else {
			RTCreateEmptyImage();
		}
	}

	void VulkanImageCube::RTDestroyResource() {
		if (!m_Image)
			return;

		//if (m_CreateInfo.ImGuiUsage)
		//	ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)m_ImGuiID);

		VulkanAllocator& allocator = m_VulkanDevice->GetAllocator();

		m_ImageView.RTDestroyResource();
		m_Sampler.RTDestroyResource();

		allocator.DestroyImage(m_Image, m_ImageVma);
		m_Image = VK_NULL_HANDLE;
	}
}