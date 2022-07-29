#include "lypch.h"
#include "VulkanImage.h"

#include "Renderer/Renderer.h"
#include "Renderer/Synchronization/VulkanSyncItems.h"

#include "../Memory/VulkanAllocator.h"
#include "../Context/VulkanDevice.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "../ThirdParty/ImGui/imgui_impl_vulkan.h"
#include "glm/gtc/integer.hpp"

namespace Lucy {

	VulkanImage2D::VulkanImage2D(const std::string& path, ImageCreateInfo& createInfo)
		: Image2D(path, createInfo) {
		if (m_CreateInfo.ImageType != ImageType::Type2D)
			LUCY_ASSERT(false);
		Renderer::EnqueueToRenderThread([=]() {
			CreateFromPath();
		});
	}

	VulkanImage2D::VulkanImage2D(ImageCreateInfo& createInfo)
		: Image2D(createInfo) {
		if (m_CreateInfo.ImageType != ImageType::Type2D)
			LUCY_ASSERT(false);
		Renderer::EnqueueToRenderThread([=]() {
			if (m_CreateInfo.Target == ImageTarget::Depth)
				CreateDepthImage();
			else
				CreateEmptyImage();
		});
	}

	void VulkanImage2D::CreateFromPath() {
		uint8_t* data = nullptr;
		data = stbi_load(m_Path.c_str(), &m_Width, &m_Height, &m_Channels, STBI_rgb_alpha);

		if (!data) {
			LUCY_CRITICAL(fmt::format("Failed to load a texture. Texture path: {0}", m_Path));
			LUCY_ASSERT(false);
		}

		if (m_Width == 0 && m_Height == 0)
			LUCY_ASSERT(false);

		uint64_t imageSize = (uint64_t)m_Width * m_Height * 4;

		VkBuffer m_ImageStagingBuffer = VK_NULL_HANDLE;
		VmaAllocation m_ImageStagingBufferVma = VK_NULL_HANDLE;

		VulkanAllocator& allocator = VulkanAllocator::Get();
		allocator.CreateVulkanBufferVma(VulkanBufferUsage::CPUOnly, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, m_ImageStagingBuffer, m_ImageStagingBufferVma);

		void* pixelData;
		vmaMapMemory(allocator.GetVmaInstance(), m_ImageStagingBufferVma, &pixelData);
		memcpy(pixelData, data, imageSize);
		vmaUnmapMemory(allocator.GetVmaInstance(), m_ImageStagingBufferVma);

		stbi_image_free(data);

		if (m_CreateInfo.GenerateMipmap)
			m_MaxMipLevel = glm::floor(glm::log2(glm::max(m_Width, m_Height))) + 1;

		VkImageUsageFlags flags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		if (m_CreateInfo.GenerateMipmap)
			flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

		allocator.CreateVulkanImageVma(m_Width, m_Height, m_MaxMipLevel, (VkFormat)m_CreateInfo.Format, m_CurrentLayout,
									   flags, VK_IMAGE_TYPE_2D, m_Image, m_ImageVma);

		TransitionImageLayout(m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyImage(m_ImageStagingBuffer);

		if (m_CreateInfo.GenerateMipmap)
			GenerateMipmaps(m_Image);
		else //transitioning only then, when we dont care about mipmapping. Mipmapping already transitions to the right layout
			TransitionImageLayout(m_Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vmaDestroyBuffer(allocator.GetVmaInstance(), m_ImageStagingBuffer, m_ImageStagingBufferVma);

		CreateVulkanImageViewHandle();
	}

	void VulkanImage2D::CreateEmptyImage() {
		if (m_Width == 0 && m_Height == 0) LUCY_ASSERT(false);

		if (m_CreateInfo.GenerateMipmap)
			m_MaxMipLevel = glm::floor(glm::log2(glm::max(m_Width, m_Height))) + 1;

		VkImageUsageFlags flags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | m_CreateInfo.AdditionalUsageFlags;

		if (m_CreateInfo.GenerateMipmap)
			flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		VulkanAllocator& allocator = VulkanAllocator::Get();
		allocator.CreateVulkanImageVma(m_Width, m_Height, m_MaxMipLevel, (VkFormat)m_CreateInfo.Format, m_CurrentLayout,
									   flags, VK_IMAGE_TYPE_2D, m_Image, m_ImageVma);

		if (m_CreateInfo.GenerateMipmap)
			GenerateMipmaps(m_Image);
		else
			m_CurrentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		CreateVulkanImageViewHandle();
	}

	void VulkanImage2D::CreateDepthImage() {
		if (m_Width == 0 && m_Height == 0)
			LUCY_ASSERT(false);

		VulkanAllocator& allocator = VulkanAllocator::Get();
		allocator.CreateVulkanImageVma(m_Width, m_Height, 1, (VkFormat)m_CreateInfo.Format, m_CurrentLayout,
									   VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_TYPE_2D, m_Image, m_ImageVma);

		m_CurrentLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		CreateVulkanImageViewHandle();
	}

	void VulkanImage2D::CreateVulkanImageViewHandle() {
		ImageViewCreateInfo imageViewCreateInfo;
		imageViewCreateInfo.Format = (VkFormat)m_CreateInfo.Format;
		imageViewCreateInfo.Image = m_Image;
		imageViewCreateInfo.ViewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.MipmapLevel = m_MaxMipLevel;
		imageViewCreateInfo.GenerateMipmap = m_CreateInfo.GenerateMipmap;
		imageViewCreateInfo.GenerateSampler = m_CreateInfo.GenerateSampler;
		imageViewCreateInfo.MagFilter = (VkFilter)m_CreateInfo.Parameter.Mag;
		imageViewCreateInfo.MinFilter = (VkFilter)m_CreateInfo.Parameter.Min;
		imageViewCreateInfo.ModeU = (VkSamplerAddressMode)m_CreateInfo.Parameter.U;
		imageViewCreateInfo.ModeV = (VkSamplerAddressMode)m_CreateInfo.Parameter.V;
		imageViewCreateInfo.ModeW = (VkSamplerAddressMode)m_CreateInfo.Parameter.W;
		imageViewCreateInfo.Target = m_CreateInfo.Target;

		m_ImageView = VulkanImageView(imageViewCreateInfo);

		if (m_CreateInfo.ImGuiUsage) {
			if (!m_ImGuiID) {
				Renderer::EnqueueToRenderThread([&]() {
					m_ImGuiID = ImGui_ImplVulkan_AddTexture(m_ImageView.GetSampler(), m_ImageView.GetVulkanHandle(), m_CurrentLayout);
				});
			} else {
				ImGui_ImplVulkanH_UpdateTexture((VkDescriptorSet)m_ImGuiID, m_ImageView.GetSampler(), m_ImageView.GetVulkanHandle(), m_CurrentLayout);
			}
		}
	}

	void VulkanImage2D::CopyImage(const VkBuffer& imageStagingBuffer) {
		Renderer::ExecuteSingleTimeCommand([&](VkCommandBuffer commandBuffer) {
			VkBufferImageCopy region{};
			region.bufferOffset = 0;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = 1;
			region.imageOffset = { 0, 0, 0 };
			region.imageExtent.width = (uint32_t)m_Width;
			region.imageExtent.height = (uint32_t)m_Height;
			region.imageExtent.depth = 1;

			vkCmdCopyBufferToImage(commandBuffer, imageStagingBuffer, m_Image, m_CurrentLayout, 1, &region);
		});
	}

	void VulkanImage2D::GenerateMipmaps(VkImage image) {

		//Transfering first mip to "src optimal" for read during vkCmdBlit
		TransitionImageLayout(image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		//Generate the mip chain
		//Copying down the whole mip chain doing a blit from mip-1 to mip
		Renderer::ExecuteSingleTimeCommand([&](VkCommandBuffer commandBuffer) {
			for (uint32_t i = 1; i < m_MaxMipLevel; i++) {
				VkImageBlit blit{};
				blit.srcSubresource.aspectMask = m_CreateInfo.Target == ImageTarget::Depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
				blit.srcSubresource.layerCount = 1;
				blit.srcSubresource.mipLevel = i - 1;
				blit.srcOffsets[1].x = (m_Width >> (i - 1));
				blit.srcOffsets[1].y = (m_Height >> (i - 1));
				blit.srcOffsets[1].z = 1;

				blit.dstSubresource.aspectMask = blit.srcSubresource.aspectMask;
				blit.dstSubresource.layerCount = 1;
				blit.dstSubresource.mipLevel = i;
				blit.dstOffsets[1].x = (m_Width >> i);
				blit.dstOffsets[1].y = (m_Height >> i);
				blit.dstOffsets[1].z = 1;

				// Prepare current mip level as image blit destination
				TransitionImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, i);

				// Blit from previous level
				vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

				// Prepare current mip level as image blit source for next level
				TransitionImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, i);
			}

			// After the loop, all mip layers are in TRANSFER_SRC layout, so transition all to SHADER_READ
			TransitionImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, m_MaxMipLevel);
		});
	}

	void VulkanImage2D::TransitionImageLayout(VkImage image, VkImageLayout newLayout, uint32_t baseMipLevel, uint32_t levelCount) {
		return TransitionImageLayout(image, m_CurrentLayout, newLayout, baseMipLevel, levelCount);
	}

	void VulkanImage2D::TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t baseMipLevel, uint32_t levelCount) {
		Renderer::ExecuteSingleTimeCommand([&](VkCommandBuffer commandBuffer) {
			TransitionImageLayout(commandBuffer, image, oldLayout, newLayout, baseMipLevel, levelCount);
		});
	}

	/// This is for mipmapping. We dont submit this to the queue. It's just a vorlage
	void VulkanImage2D::TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t baseMipLevel, uint32_t levelCount) {
		ImageMemoryBarrierCreateInfo createInfo;
		createInfo.ImageHandle = image;

		VkImageSubresourceRange subresourceRange;
		subresourceRange.aspectMask = m_CreateInfo.Target == ImageTarget::Depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = baseMipLevel;
		subresourceRange.levelCount = levelCount;
		subresourceRange.layerCount = 1;
		subresourceRange.baseArrayLayer = 0;

		createInfo.SubResourceRange = subresourceRange;
		createInfo.OldLayout = oldLayout;
		createInfo.NewLayout = newLayout;

		ImageMemoryBarrier barrier(createInfo);
		barrier.RunBarrier(commandBuffer);

		m_CurrentLayout = newLayout;
	}

	void VulkanImage2D::Destroy() {
		if (!m_Image)
			return;
		m_ImageView.Destroy();

		VulkanAllocator& allocator = VulkanAllocator::Get();
		vmaDestroyImage(allocator.GetVmaInstance(), m_Image, m_ImageVma);
		m_Image = VK_NULL_HANDLE;
	}

	void VulkanImage2D::Recreate(uint32_t width, uint32_t height) {
		m_Width = width;
		m_Height = height;

		m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		Destroy();

		if (!m_Path.empty())
			CreateFromPath();
		else if (m_CreateInfo.Target == ImageTarget::Depth)
			CreateDepthImage();
		else
			CreateEmptyImage();
	}

	VulkanImageView::VulkanImageView(const ImageViewCreateInfo& createInfo)
		: m_CreateInfo(createInfo) {
		CreateView();
		if (m_CreateInfo.GenerateSampler)
			CreateSampler();
	}

	void VulkanImageView::CreateView() {
		const VulkanDevice& device = VulkanDevice::Get();

		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = m_CreateInfo.Image;
		createInfo.viewType = m_CreateInfo.ViewType;
		createInfo.format = m_CreateInfo.Format;

		if (m_CreateInfo.Target == ImageTarget::Depth)
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		else
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = m_CreateInfo.MipmapLevel;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		createInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };

		LUCY_VK_ASSERT(vkCreateImageView(device.GetLogicalDevice(), &createInfo, nullptr, &m_ImageView));
	}

	void VulkanImageView::CreateSampler() {
		const VulkanDevice& device = VulkanDevice::Get();

		VkSamplerCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		createInfo.magFilter = m_CreateInfo.MagFilter;
		createInfo.minFilter = m_CreateInfo.MinFilter;
		createInfo.addressModeU = m_CreateInfo.ModeU;
		createInfo.addressModeV = m_CreateInfo.ModeV;
		createInfo.addressModeW = m_CreateInfo.ModeW;

		createInfo.anisotropyEnable = VK_TRUE;

		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(device.GetPhysicalDevice(), &properties);

		createInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy; //best quality, TODO: in the future, add an option
		createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		createInfo.unnormalizedCoordinates = VK_FALSE;
		//TODO: for pcf
		createInfo.compareEnable = VK_FALSE;
		createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		createInfo.mipLodBias = 0.0f;
		createInfo.minLod = 0.0f;

		if (!m_CreateInfo.GenerateMipmap) {
			createInfo.maxLod = 0.0f;
		} else {
			createInfo.maxLod = m_CreateInfo.MipmapLevel;
		}

		LUCY_VK_ASSERT(vkCreateSampler(device.GetLogicalDevice(), &createInfo, nullptr, &m_Sampler));
	}

	void VulkanImageView::Recreate(const ImageViewCreateInfo& createInfo) {
		m_CreateInfo = createInfo;
		Destroy();
		CreateView();
		if (m_CreateInfo.GenerateSampler)
			CreateSampler();
	}

	void VulkanImageView::Destroy() {
		vkDestroyImageView(VulkanDevice::Get().GetLogicalDevice(), m_ImageView, nullptr);
		if (m_CreateInfo.GenerateSampler)
			vkDestroySampler(VulkanDevice::Get().GetLogicalDevice(), m_Sampler, nullptr);
	}
}