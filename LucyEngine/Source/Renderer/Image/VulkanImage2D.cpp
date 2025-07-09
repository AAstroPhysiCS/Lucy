#include "lypch.h"
#include "VulkanImage2D.h"

#include "Renderer/Renderer.h"
#include "Renderer/Device/VulkanRenderDevice.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "../../../ThirdParty/ImGui/imgui_impl_vulkan.h"

namespace Lucy {

	VulkanImage2D::VulkanImage2D(const std::filesystem::path& path, const ImageCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device)
		: VulkanImage(path, createInfo), m_VulkanDevice(device) {
		if (m_CreateInfo.ImageType != ImageType::Type2D)
			LUCY_ASSERT(false);

		RTCreateFromPath();
	}

	VulkanImage2D::VulkanImage2D(const ImageCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device)
		: VulkanImage(createInfo), m_VulkanDevice(device) {
		if (m_CreateInfo.ImageType != ImageType::Type2D)
			LUCY_ASSERT(false);

		if (m_CreateInfo.ImageUsage == ImageUsage::AsDepthAttachment)
			RTCreateDepthImage();
		else
			RTCreateEmptyImage();
	}

	VulkanImage2D::VulkanImage2D(const Ref<VulkanImage2D>& other, const Ref<VulkanRenderDevice>& device)
		: VulkanImage(*other), m_VulkanDevice(device) {
		m_CreateInfo = other->m_CreateInfo;
		m_Path = other->m_Path;
		m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		m_ImGuiID = 0;

		if (m_CreateInfo.ImageType != ImageType::Type2D)
			LUCY_ASSERT(false);

		if (!other->m_Path.empty()) {
			RTCreateFromPath();
			return;
		}

		if (m_CreateInfo.ImageUsage == ImageUsage::AsDepthAttachment)
			RTCreateDepthImage();
		else
			RTCreateEmptyImage();
	}

	void VulkanImage2D::RTCreateFromPath() {
		std::string pathInString = m_Path.string();
		uint8_t* data = nullptr;
		bool isHDR = stbi_is_hdr(pathInString.c_str());

		if (isHDR)
			data = (uint8_t*)stbi_loadf(pathInString.c_str(), (int32_t*)&m_CreateInfo.Width, (int32_t*)&m_CreateInfo.Height, &m_Channels, STBI_rgb_alpha);
		else
			data = stbi_load(pathInString.c_str(), (int32_t*)&m_CreateInfo.Width, (int32_t*)&m_CreateInfo.Height, &m_Channels, STBI_rgb_alpha);

		if (m_CreateInfo.GenerateMipmap)
			m_MaxMipLevel = (uint32_t)glm::floor(glm::log2(glm::max(m_CreateInfo.Width, m_CreateInfo.Height))) + 1u;

		LUCY_ASSERT(data != nullptr, "Failed to load a texture. Texture path: {0}", pathInString);
		LUCY_ASSERT(m_CreateInfo.Width > 0 && m_CreateInfo.Height > 0, "Width or height of the image is less than zero.");

		VkDeviceSize imageSize = (VkDeviceSize)m_CreateInfo.Width * m_CreateInfo.Height * 4 * GetFormatSize(m_CreateInfo.Format);

		VkBuffer imageStagingBuffer = VK_NULL_HANDLE;
		VmaAllocation imageStagingBufferVma = VK_NULL_HANDLE;

		VulkanAllocator& allocator = m_VulkanDevice->GetAllocator();
		allocator.CreateVulkanBufferVma(VulkanBufferUsage::CPUOnly, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, imageStagingBuffer, imageStagingBufferVma);

		void* pixelData = nullptr;
		allocator.MapMemory(imageStagingBufferVma, pixelData);
		if (isHDR)
			memcpy(pixelData, (float*)data, imageSize);
		else
			memcpy(pixelData, data, imageSize);
		allocator.UnmapMemory(imageStagingBufferVma);

		stbi_image_free(data);

		VkImageUsageFlags flags = GetImageFlagsBasedOnUsage();

		allocator.CreateVulkanImageVma(m_CreateInfo.Width, m_CreateInfo.Height, m_MaxMipLevel, (VkFormat)GetAPIImageFormat(m_CreateInfo.Format), m_CurrentLayout,
									   flags, VK_IMAGE_TYPE_2D, m_Image, m_ImageVma);

		TransitionImageLayoutImmediate(m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyBufferToImageImmediate(imageStagingBuffer);

		if (m_CreateInfo.GenerateMipmap)
			GenerateMipmapsImmediate();
		else //transitioning only then, when we dont care about mipmapping. Mipmapping already transitions to the right layout
			SetLayoutImmediate(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		allocator.DestroyBuffer(imageStagingBuffer, imageStagingBufferVma);

		RTCreateVulkanImageViewHandle(m_VulkanDevice);
	}

	void VulkanImage2D::RTCreateEmptyImage() {
		LUCY_ASSERT(m_CreateInfo.Width > 0 && m_CreateInfo.Height > 0, "Width or height of the image is less than zero.");

		if (m_CreateInfo.GenerateMipmap)
			m_MaxMipLevel = (uint32_t)glm::floor(glm::log2(glm::max(m_CreateInfo.Width, m_CreateInfo.Height))) + 1u;

		VkImageUsageFlags flags = GetImageFlagsBasedOnUsage();

		VulkanAllocator& allocator = m_VulkanDevice->GetAllocator();
		allocator.CreateVulkanImageVma(m_CreateInfo.Width, m_CreateInfo.Height, m_MaxMipLevel, (VkFormat)GetAPIImageFormat(m_CreateInfo.Format), m_CurrentLayout,
									   flags, VK_IMAGE_TYPE_2D, m_Image, m_ImageVma, 0, m_CreateInfo.Layers);

		if (m_CreateInfo.GenerateMipmap)
			GenerateMipmapsImmediate();
		else
			SetLayoutImmediate(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		RTCreateVulkanImageViewHandle(m_VulkanDevice);
	}

	void VulkanImage2D::RTCreateDepthImage() {
		LUCY_ASSERT(m_CreateInfo.Width > 0 && m_CreateInfo.Height > 0, "Width or height of the image is less than zero.");

		if (m_CreateInfo.GenerateMipmap)
			m_MaxMipLevel = (uint32_t)glm::floor(glm::log2(glm::max(m_CreateInfo.Width, m_CreateInfo.Height))) + 1u;
		//do the flags
		VkImageUsageFlags flags = GetImageFlagsBasedOnUsage();

		VulkanAllocator& allocator = m_VulkanDevice->GetAllocator();
		allocator.CreateVulkanImageVma(m_CreateInfo.Width, m_CreateInfo.Height, 1, (VkFormat)GetAPIImageFormat(m_CreateInfo.Format), m_CurrentLayout, 
			flags, VK_IMAGE_TYPE_2D, m_Image, m_ImageVma, 0U, m_CreateInfo.Layers);

		if (m_CreateInfo.GenerateMipmap)
			GenerateMipmapsImmediate();
		else
			SetLayoutImmediate(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

		RTCreateVulkanImageViewHandle(m_VulkanDevice);
	}

	void VulkanImage2D::RTDestroyResource() {
		if (!m_Image)
			return;

		//if (m_CreateInfo.ImGuiUsage)
			//ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)m_ImGuiID);

		m_ImageView.RTDestroyResource();

		VulkanAllocator& allocator = m_VulkanDevice->GetAllocator();
		allocator.DestroyImage(m_Image, m_ImageVma);
		m_Image = VK_NULL_HANDLE;
	}

	void VulkanImage2D::RTRecreate(uint32_t width, uint32_t height) {
		m_CreateInfo.Width = width;
		m_CreateInfo.Height = height;

		m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		RTDestroyResource();

		if (!m_Path.empty())
			RTCreateFromPath();
		else if (m_CreateInfo.ImageUsage == ImageUsage::AsDepthAttachment)
			RTCreateDepthImage();
		else
			RTCreateEmptyImage();
	}
}