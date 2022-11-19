#include "lypch.h"
#include "VulkanImage2D.h"

#include "Renderer/Renderer.h"
#include "Renderer/Memory/VulkanAllocator.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

namespace Lucy {

	VulkanImage2D::VulkanImage2D(const std::string& path, ImageCreateInfo& createInfo)
		: VulkanImage(path, createInfo) {
		if (m_CreateInfo.ImageType != ImageType::Type2DColor &&
			m_CreateInfo.ImageType != ImageType::Type2DDepth &&
			m_CreateInfo.ImageType != ImageType::Type2DArrayColor)
			LUCY_ASSERT(false);

		Renderer::EnqueueToRenderThread([=]() {
			CreateFromPath();
		});
	}

	VulkanImage2D::VulkanImage2D(ImageCreateInfo& createInfo)
		: VulkanImage(createInfo) {
		if (m_CreateInfo.ImageType != ImageType::Type2DColor && 
			m_CreateInfo.ImageType != ImageType::Type2DDepth &&
			m_CreateInfo.ImageType != ImageType::Type2DArrayColor)
			LUCY_ASSERT(false);

		Renderer::EnqueueToRenderThread([=]() {
			if (m_CreateInfo.ImageType == ImageType::Type2DDepth)
				CreateDepthImage();
			else
				CreateEmptyImage();
		});
	}

	VulkanImage2D::VulkanImage2D(const Ref<VulkanImage2D>& other)
		: VulkanImage(*other.Get()) {
		m_CreateInfo = other->m_CreateInfo;
		m_Path = other->m_Path;
		m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		m_ImGuiID = 0;
		m_LayerCount = other->m_LayerCount;

		if (other->m_CreateInfo.ImageType != ImageType::Type2DColor &&
			other->m_CreateInfo.ImageType != ImageType::Type2DDepth &&
			other->m_CreateInfo.ImageType != ImageType::Type2DArrayColor)
			LUCY_ASSERT(false);

		if (!other->m_Path.empty()) {
			CreateFromPath();
			return;
		}

		if (other->m_CreateInfo.ImageType == ImageType::Type2DDepth)
			CreateDepthImage();
		else
			CreateEmptyImage();
	}

	void VulkanImage2D::CreateFromPath() {
		uint8_t* data = nullptr;
		bool isHDR = stbi_is_hdr(m_Path.c_str());

		if (isHDR)
			data = (uint8_t*)stbi_loadf(m_Path.c_str(), &m_Width, &m_Height, &m_Channels, STBI_rgb_alpha);
		else
			data = stbi_load(m_Path.c_str(), &m_Width, &m_Height, &m_Channels, STBI_rgb_alpha);

		if (m_CreateInfo.GenerateMipmap)
			m_MaxMipLevel = glm::floor(glm::log2(glm::max(m_Width, m_Height))) + 1;

		if (!data) {
			LUCY_CRITICAL(fmt::format("Failed to load a texture. Texture path: {0}", m_Path));
			LUCY_ASSERT(false);
		}

		if (m_Width == 0 && m_Height == 0)
			LUCY_ASSERT(false);

		uint64_t imageSize = (uint64_t)m_Width * m_Height * 4 * GetFormatSize(m_CreateInfo.Format);

		VkBuffer imageStagingBuffer = VK_NULL_HANDLE;
		VmaAllocation imageStagingBufferVma = VK_NULL_HANDLE;

		VulkanAllocator& allocator = VulkanAllocator::Get();
		allocator.CreateVulkanBufferVma(VulkanBufferUsage::CPUOnly, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, imageStagingBuffer, imageStagingBufferVma);

		void* pixelData = nullptr;
		allocator.MapMemory(imageStagingBufferVma, pixelData);
		if (isHDR)
			memcpy(pixelData, (float*)data, imageSize);
		else
			memcpy(pixelData, data, imageSize);
		allocator.UnmapMemory(imageStagingBufferVma);

		stbi_image_free(data);

		VkImageUsageFlags flags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | m_CreateInfo.Flags;

		if (m_CreateInfo.GenerateSampler)
			flags |= VK_IMAGE_USAGE_SAMPLED_BIT;

		if (m_CreateInfo.GenerateMipmap)
			flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

		allocator.CreateVulkanImageVma(m_Width, m_Height, m_MaxMipLevel, (VkFormat)GetAPIImageFormat(m_CreateInfo.Format), m_CurrentLayout,
									   flags, VK_IMAGE_TYPE_2D, m_Image, m_ImageVma);

		TransitionImageLayout(m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyBufferToImage(imageStagingBuffer);

		if (m_CreateInfo.GenerateMipmap)
			GenerateMipmaps();
		else //transitioning only then, when we dont care about mipmapping. Mipmapping already transitions to the right layout
			SetLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		allocator.DestroyBuffer(imageStagingBuffer, imageStagingBufferVma);

		CreateVulkanImageViewHandle();
	}

	void VulkanImage2D::CreateEmptyImage() {
		if (m_Width == 0 && m_Height == 0)
			LUCY_ASSERT(false);

		if (m_CreateInfo.GenerateMipmap)
			m_MaxMipLevel = glm::floor(glm::log2(glm::max(m_Width, m_Height))) + 1;

		VkImageUsageFlags flags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | m_CreateInfo.Flags;

		if (m_CreateInfo.GenerateSampler)
			flags |= VK_IMAGE_USAGE_SAMPLED_BIT;

		if (m_CreateInfo.GenerateMipmap)
			flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		VulkanAllocator& allocator = VulkanAllocator::Get();
		allocator.CreateVulkanImageVma(m_Width, m_Height, m_MaxMipLevel, (VkFormat)GetAPIImageFormat(m_CreateInfo.Format), m_CurrentLayout,
									   flags, VK_IMAGE_TYPE_2D, m_Image, m_ImageVma, 0, m_CreateInfo.Layers);

		if (m_CreateInfo.GenerateMipmap)
			GenerateMipmaps();
		else
			SetLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		CreateVulkanImageViewHandle();
	}

	void VulkanImage2D::CreateDepthImage() {
		if (m_Width == 0 && m_Height == 0)
			LUCY_ASSERT(false);

		VulkanAllocator& allocator = VulkanAllocator::Get();
		allocator.CreateVulkanImageVma(m_Width, m_Height, 1, (VkFormat)GetAPIImageFormat(m_CreateInfo.Format), m_CurrentLayout,
									   VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_TYPE_2D, m_Image, m_ImageVma);

		m_CurrentLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		CreateVulkanImageViewHandle();
	}

	void VulkanImage2D::Destroy() {
		if (!m_Image)
			return;
		m_ImageView.Destroy();

		VulkanAllocator::Get().DestroyImage(m_Image, m_ImageVma);
		m_Image = VK_NULL_HANDLE;
	}

	void VulkanImage2D::Recreate(uint32_t width, uint32_t height) {
		m_Width = width;
		m_Height = height;

		m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		Destroy();

		if (!m_Path.empty())
			CreateFromPath();
		else if (m_CreateInfo.ImageType == ImageType::Type2DDepth)
			CreateDepthImage();
		else
			CreateEmptyImage();
	}
}