#include "lypch.h"
#include "VulkanImage.h"

#include "Renderer/Renderer.h"
#include "Renderer/Synchronization/VulkanSyncItems.h"
#include "Renderer/Context/VulkanContextDevice.h"

#include "../ThirdParty/ImGui/imgui_impl_vulkan.h"

namespace Lucy {

	VulkanImage::VulkanImage(const std::string& path, ImageCreateInfo& createInfo)
		: Image(path, createInfo) {
	}

	VulkanImage::VulkanImage(ImageCreateInfo& createInfo)
		: Image(createInfo) {
	}

	void VulkanImage::CopyImageToImage(VkImage image, VkImageLayout layout, const std::vector<VkImageCopy>& regions) {
		Renderer::SubmitImmediateCommand([=](VkCommandBuffer commandBuffer) {
			vkCmdCopyImage(commandBuffer, m_Image, m_CurrentLayout, image, layout, regions.size(), regions.data());
		});
	}

	void VulkanImage::CopyImageToBuffer(VkImage image, const VkBuffer& bufferToCopy, uint32_t layerCount) {
		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = layerCount;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent.width = (uint32_t)m_Width;
		region.imageExtent.height = (uint32_t)m_Height;
		region.imageExtent.depth = 1;

		CopyImageToBuffer(image, bufferToCopy, { region });
	}

	void VulkanImage::CopyBufferToImage(VkImage image, const VkBuffer& bufferToCopy, uint32_t layerCount) {
		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = layerCount;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent.width = (uint32_t)m_Width;
		region.imageExtent.height = (uint32_t)m_Height;
		region.imageExtent.depth = 1;

		CopyBufferToImage(image, bufferToCopy, { region });
	}

	void VulkanImage::CopyImageToBuffer(const VkBuffer& bufferToCopy, uint32_t layerCount) {
		CopyImageToBuffer(m_Image, bufferToCopy, layerCount);
	}

	void VulkanImage::CopyBufferToImage(const VkBuffer& bufferToCopy, uint32_t layerCount) {
		CopyBufferToImage(m_Image, bufferToCopy, layerCount);
	}

	void VulkanImage::CopyImageToBuffer(const VkBuffer& bufferToCopy, const std::vector<VkBufferImageCopy>& imageCopyRegions) {
		CopyImageToBuffer(m_Image, bufferToCopy, imageCopyRegions);
	}

	void VulkanImage::CopyBufferToImage(const VkBuffer& bufferToCopy, const std::vector<VkBufferImageCopy>& bufferCopyRegions) {
		CopyBufferToImage(m_Image, bufferToCopy, bufferCopyRegions);
	}

	void VulkanImage::CopyImageToBuffer(VkImage image, const VkBuffer& bufferToCopy, const std::vector<VkBufferImageCopy>& imageCopyRegions) {
		Renderer::SubmitImmediateCommand([=](VkCommandBuffer commandBuffer) {
			vkCmdCopyImageToBuffer(commandBuffer, image, m_CurrentLayout, bufferToCopy, imageCopyRegions.size(), imageCopyRegions.data());
		});
	}

	void VulkanImage::CopyBufferToImage(VkImage image, const VkBuffer& bufferToCopy, const std::vector<VkBufferImageCopy>& bufferCopyRegions) {
		Renderer::SubmitImmediateCommand([=](VkCommandBuffer commandBuffer) {
			vkCmdCopyBufferToImage(commandBuffer, bufferToCopy, image, m_CurrentLayout, bufferCopyRegions.size(), bufferCopyRegions.data());
		});
	}

	void VulkanImage::GenerateMipmaps() {
		GenerateMipmaps(m_Image);
	}

	void VulkanImage::GenerateMipmaps(VkImage image) {
		//Transfering first mip to "src optimal" for read during vkCmdBlit
		TransitionImageLayout(image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		//Generate the mip chain
		//Copying down the whole mip chain doing a blit from mip-1 to mip
		Renderer::SubmitImmediateCommand([&](VkCommandBuffer commandBuffer) {
			for (uint32_t i = 1; i < m_MaxMipLevel; i++) {
				VkImageBlit blit{};
				blit.srcSubresource.aspectMask = m_CreateInfo.ImageType == ImageType::Type2DDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
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

	void VulkanImage::SetLayout(VkImageLayout newLayout) {
		TransitionImageLayout(m_Image, newLayout);
	}

	void VulkanImage::SetLayout(VkImageLayout newLayout, uint32_t baseMipLevel, uint32_t levelCount, uint32_t layerCount) {
		TransitionImageLayout(m_Image, newLayout, baseMipLevel, levelCount, layerCount);
	}

	void VulkanImage::CopyImageToImage(const Ref<VulkanImage>& imageToCopy, const std::vector<VkImageCopy>& imageCopyRegions) {
		CopyImageToImage(imageToCopy->GetVulkanHandle(), imageToCopy->GetCurrentLayout(), imageCopyRegions);
	}

	void VulkanImage::CopyImageToImage(const VulkanImage* imageToCopy, const std::vector<VkImageCopy>& imageCopyRegions) {
		CopyImageToImage(imageToCopy->GetVulkanHandle(), imageToCopy->GetCurrentLayout(), imageCopyRegions);
	}

	void VulkanImage::TransitionImageLayout(VkImage image, VkImageLayout newLayout, uint32_t baseMipLevel, uint32_t levelCount, uint32_t layerCount) {
		return TransitionImageLayout(image, m_CurrentLayout, newLayout, baseMipLevel, levelCount, layerCount);
	}

	void VulkanImage::TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t baseMipLevel, uint32_t levelCount, uint32_t layerCount) {
		Renderer::SubmitImmediateCommand([&](VkCommandBuffer commandBuffer) {
			TransitionImageLayout(commandBuffer, image, oldLayout, newLayout, baseMipLevel, levelCount, layerCount);
		});
	}

	/// This is for mipmapping. We dont submit this to the queue. It's just a vorlage
	void VulkanImage::TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t baseMipLevel, uint32_t levelCount, uint32_t layerCount) {
		ImageMemoryBarrierCreateInfo createInfo;
		createInfo.ImageHandle = image;

		VkImageSubresourceRange subresourceRange{};
		subresourceRange.aspectMask = m_CreateInfo.ImageType == ImageType::Type2DDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = baseMipLevel;
		subresourceRange.levelCount = levelCount;
		subresourceRange.layerCount = layerCount;
		subresourceRange.baseArrayLayer = 0;

		createInfo.SubResourceRange = subresourceRange;
		createInfo.OldLayout = oldLayout;
		createInfo.NewLayout = newLayout;

		ImageMemoryBarrier barrier(createInfo);
		barrier.RunBarrier(commandBuffer);

		m_CurrentLayout = newLayout;
	}

	void VulkanImage::CreateVulkanImageViewHandle() {
		auto GetImageFilter = [](ImageFilterMode mode) {
			switch (mode) {
				case ImageFilterMode::LINEAR:
					return VK_FILTER_LINEAR;
				case ImageFilterMode::NEAREST:
					return VK_FILTER_NEAREST;
				default:
					return VK_FILTER_MAX_ENUM;
			}
		};

		auto GetImageAddressMode = [](ImageAddressMode mode) {
			switch (mode) {
				case ImageAddressMode::REPEAT:
					return VK_SAMPLER_ADDRESS_MODE_REPEAT;
				case ImageAddressMode::CLAMP_TO_BORDER:
					return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
				case ImageAddressMode::CLAMP_TO_EDGE:
					return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				default:
					return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
			}
		};

		ImageViewCreateInfo imageViewCreateInfo {
			.Image = m_Image,
			.ImageType = m_CreateInfo.ImageType,
			.Format = (VkFormat)GetAPIImageFormat(m_CreateInfo.Format),
			.GenerateSampler = m_CreateInfo.GenerateSampler,
			.GenerateMipmap = m_CreateInfo.GenerateMipmap,
			.MipmapLevel = m_MaxMipLevel,
			.Layers = m_CreateInfo.Layers,
			.MagFilter = GetImageFilter(m_CreateInfo.Parameter.Mag),
			.MinFilter = GetImageFilter(m_CreateInfo.Parameter.Min),
			.ModeU = GetImageAddressMode(m_CreateInfo.Parameter.U),
			.ModeV = GetImageAddressMode(m_CreateInfo.Parameter.V),
			.ModeW = GetImageAddressMode(m_CreateInfo.Parameter.W)
		};

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

	VulkanImageView::VulkanImageView(const ImageViewCreateInfo& createInfo)
		: m_CreateInfo(createInfo) {
		CreateView();
		if (m_CreateInfo.GenerateSampler)
			CreateSampler();
	}

	void VulkanImageView::CreateView() {
		auto GetImageType = [](ImageType type) {
			switch (type) {
				case ImageType::Type2DDepth:
				case ImageType::Type2DColor:
					return VK_IMAGE_VIEW_TYPE_2D;
				case ImageType::Type2DArrayColor:
					return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
				case ImageType::Type3DColor:
					return VK_IMAGE_VIEW_TYPE_3D;
				case ImageType::TypeCubeColor:
					return VK_IMAGE_VIEW_TYPE_CUBE;
				default:
					return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
			}
		};

		auto GetLayerCount = [this](ImageType type) {
			switch (type) {
				case ImageType::Type2DDepth:
				case ImageType::Type2DColor:
					return 1u;
				case ImageType::Type2DArrayColor:
					return m_CreateInfo.Layers;
				case ImageType::Type3DColor:
					return 3u;
				case ImageType::TypeCubeColor:
					return 6u;
				default:
					return 1u;
			}
		};

		const VulkanContextDevice& device = VulkanContextDevice::Get();

		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = m_CreateInfo.Image;
		createInfo.viewType = GetImageType(m_CreateInfo.ImageType);
		createInfo.format = m_CreateInfo.Format;

		if (m_CreateInfo.ImageType == ImageType::Type2DDepth)
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		else
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = m_CreateInfo.MipmapLevel;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = GetLayerCount(m_CreateInfo.ImageType);
		createInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };

		LUCY_VK_ASSERT(vkCreateImageView(device.GetLogicalDevice(), &createInfo, nullptr, &m_ImageView));
	}

	void VulkanImageView::CreateSampler() {
		const VulkanContextDevice& device = VulkanContextDevice::Get();

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
		vkDestroyImageView(VulkanContextDevice::Get().GetLogicalDevice(), m_ImageView, nullptr);
		if (m_CreateInfo.GenerateSampler)
			vkDestroySampler(VulkanContextDevice::Get().GetLogicalDevice(), m_Sampler, nullptr);
	}

	uint32_t GetAPIImageFormat(ImageFormat format) {
		if (Renderer::GetRenderArchitecture() == RenderArchitecture::Vulkan) {
			switch (format) {
				case ImageFormat::R8G8B8A8_UNORM:
					return VK_FORMAT_R8G8B8A8_UNORM;
				case ImageFormat::R8G8B8A8_UINT:
					return VK_FORMAT_R8G8B8A8_UINT;
				case ImageFormat::R8G8B8A8_SRGB:
					return VK_FORMAT_R8G8B8A8_SRGB;
				case ImageFormat::B8G8R8A8_SRGB:
					return VK_FORMAT_B8G8R8A8_SRGB;
				case ImageFormat::B8G8R8A8_UINT:
					return VK_FORMAT_B8G8R8A8_UINT;
				case ImageFormat::B8G8R8A8_UNORM:
					return VK_FORMAT_B8G8R8A8_UNORM;
				case ImageFormat::D32_SFLOAT:
					return VK_FORMAT_D32_SFLOAT;
				case ImageFormat::R16G16B16A16_SFLOAT:
					return VK_FORMAT_R16G16B16A16_SFLOAT;
				case ImageFormat::R16G16B16A16_UINT:
					return VK_FORMAT_R16G16B16A16_UINT;
				case ImageFormat::R16G16B16A16_UNORM:
					return VK_FORMAT_R16G16B16A16_UNORM;
				case ImageFormat::R32G32B32A32_SFLOAT:
					return VK_FORMAT_R32G32B32A32_SFLOAT;
				case ImageFormat::R32G32B32A32_UINT:
					return VK_FORMAT_R32G32B32A32_UINT;
				case ImageFormat::R32G32B32_SFLOAT:
					return VK_FORMAT_R32G32B32_SFLOAT;
				case ImageFormat::R32_SFLOAT:
					return VK_FORMAT_R32_SFLOAT;
				case ImageFormat::R32_UINT:
					return VK_FORMAT_R32_UINT;
				default:
					return VK_FORMAT_MAX_ENUM;
			}
		}
	}

	ImageFormat GetLucyImageFormat(uint32_t format) {
		if (Renderer::GetRenderArchitecture() == RenderArchitecture::Vulkan) {
			switch (format) {
				case VK_FORMAT_R8G8B8A8_UNORM:
					return ImageFormat::R8G8B8A8_UNORM;
				case VK_FORMAT_R8G8B8A8_UINT:
					return ImageFormat::R8G8B8A8_UINT;
				case VK_FORMAT_R8G8B8A8_SRGB:
					return ImageFormat::R8G8B8A8_SRGB;
				case VK_FORMAT_B8G8R8A8_SRGB:
					return ImageFormat::B8G8R8A8_SRGB;
				case VK_FORMAT_B8G8R8A8_UINT:
					return ImageFormat::B8G8R8A8_UINT;
				case VK_FORMAT_B8G8R8A8_UNORM:
					return ImageFormat::B8G8R8A8_UNORM;
				case VK_FORMAT_D32_SFLOAT:
					return ImageFormat::D32_SFLOAT;
				case VK_FORMAT_R16G16B16A16_SFLOAT:
					return ImageFormat::R16G16B16A16_SFLOAT;
				case VK_FORMAT_R16G16B16A16_UINT:
					return ImageFormat::R16G16B16A16_UINT;
				case VK_FORMAT_R16G16B16A16_UNORM:
					return ImageFormat::R16G16B16A16_UNORM;
				case VK_FORMAT_R32G32B32A32_SFLOAT:
					return ImageFormat::R32G32B32A32_SFLOAT;
				case VK_FORMAT_R32G32B32A32_UINT:
					return ImageFormat::R32G32B32A32_UINT;
				case VK_FORMAT_R32G32B32_SFLOAT:
					return ImageFormat::R32G32B32_SFLOAT;
				case VK_FORMAT_R32_SFLOAT:
					return ImageFormat::R32_SFLOAT;
				case VK_FORMAT_R32_UINT:
					return ImageFormat::R32_UINT;
				default:
					return ImageFormat::Unknown;
			}
		}
	}
}