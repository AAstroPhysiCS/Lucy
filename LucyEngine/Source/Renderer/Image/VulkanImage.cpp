#include "lypch.h"
#include "VulkanImage.h"

#include "Renderer/Renderer.h"
#include "Renderer/Synchronization/VulkanSyncItems.h"
#include "Renderer/Device/VulkanRenderDevice.h"

#include "../ThirdParty/ImGui/imgui_impl_vulkan.h"

namespace Lucy {

	VulkanImage::VulkanImage(const std::filesystem::path& path, const ImageCreateInfo& createInfo)
		: Image(path, createInfo) {
	}

	VulkanImage::VulkanImage(const ImageCreateInfo& createInfo)
		: Image(createInfo) {
	}

	void VulkanImage::CopyImageToImageImmediate(const Ref<VulkanImage>& destImage, const std::vector<VkImageCopy>& imageCopyRegions) {
		CopyImageToImageImmediate(destImage->GetVulkanHandle(), destImage->GetCurrentLayout(), imageCopyRegions);
	}

	void VulkanImage::CopyImageToImageImmediate(const VulkanImage* destImage, const std::vector<VkImageCopy>& imageCopyRegions) {
		CopyImageToImageImmediate(destImage->GetVulkanHandle(), destImage->GetCurrentLayout(), imageCopyRegions);
	}

	void VulkanImage::CopyImageToImage(VkCommandBuffer commandBuffer, const Ref<VulkanImage>& destImage, const std::vector<VkImageCopy>& imageCopyRegions) {
		vkCmdCopyImage(commandBuffer, m_Image, m_CurrentLayout, destImage->GetVulkanHandle(), destImage->GetCurrentLayout(), (uint32_t)imageCopyRegions.size(), imageCopyRegions.data());
	}

	void VulkanImage::CopyImageToImageImmediate(VkImage image, VkImageLayout layout, const std::vector<VkImageCopy>& regions) {
		Renderer::SubmitImmediateCommand([=](VkCommandBuffer commandBuffer) {
			vkCmdCopyImage(commandBuffer, m_Image, m_CurrentLayout, image, layout, (uint32_t)regions.size(), regions.data());
		});
	}

	void VulkanImage::CopyImageToBufferImmediate(VkImage image, const VkBuffer& bufferToCopy, uint32_t layerCount) {
		VkImageSubresourceLayers imageSubresource = VulkanAPI::ImageSubresourceLayers(VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, layerCount);
		VkBufferImageCopy region = VulkanAPI::BufferImageCopy(0, 0, 0, imageSubresource, { 0, 0, 0 }, { (uint32_t)m_CreateInfo.Width, (uint32_t)m_CreateInfo.Height, 1 });

		CopyImageToBufferImmediate(image, bufferToCopy, { region });
	}

	void VulkanImage::CopyImageToBufferImmediate(const VkBuffer& bufferToCopy, uint32_t layerCount) {
		CopyImageToBufferImmediate(m_Image, bufferToCopy, layerCount);
	}

	void VulkanImage::CopyImageToBufferImmediate(const VkBuffer& bufferToCopy, const std::vector<VkBufferImageCopy>& imageCopyRegions) {
		CopyImageToBufferImmediate(m_Image, bufferToCopy, imageCopyRegions);
	}

	void VulkanImage::CopyImageToBufferImmediate(VkImage image, const VkBuffer& bufferToCopy, const std::vector<VkBufferImageCopy>& imageCopyRegions) {
		Renderer::SubmitImmediateCommand([=](VkCommandBuffer commandBuffer) {
			vkCmdCopyImageToBuffer(commandBuffer, image, m_CurrentLayout, bufferToCopy, (uint32_t)imageCopyRegions.size(), imageCopyRegions.data());
		});
	}

	void VulkanImage::CopyBufferToImageImmediate(VkImage image, const VkBuffer& bufferToCopy, uint32_t layerCount) {
		VkImageSubresourceLayers imageSubresource = VulkanAPI::ImageSubresourceLayers(VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, layerCount);
		VkBufferImageCopy region = VulkanAPI::BufferImageCopy(0, 0, 0, imageSubresource, { 0, 0, 0 }, { (uint32_t)m_CreateInfo.Width, (uint32_t)m_CreateInfo.Height, 1 });

		CopyBufferToImageImmediate(image, bufferToCopy, { region });
	}

	void VulkanImage::CopyBufferToImageImmediate(const VkBuffer& bufferToCopy, uint32_t layerCount) {
		CopyBufferToImageImmediate(m_Image, bufferToCopy, layerCount);
	}

	void VulkanImage::CopyBufferToImageImmediate(const VkBuffer& bufferToCopy, const std::vector<VkBufferImageCopy>& bufferCopyRegions) {
		CopyBufferToImageImmediate(m_Image, bufferToCopy, bufferCopyRegions);
	}

	void VulkanImage::CopyBufferToImageImmediate(VkImage image, const VkBuffer& bufferToCopy, const std::vector<VkBufferImageCopy>& bufferCopyRegions) {
		Renderer::SubmitImmediateCommand([=](VkCommandBuffer commandBuffer) {
			vkCmdCopyBufferToImage(commandBuffer, bufferToCopy, image, m_CurrentLayout, (uint32_t)bufferCopyRegions.size(), bufferCopyRegions.data());
		});
	}

	void VulkanImage::GenerateMipmapsImmediate() {
		//Transfering first mip of all the layers (if it has any) to "src optimal" for read during vkCmdBlit
		TransitionImageLayoutImmediate(m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0, 0, 1, m_CreateInfo.Layers);

		//Generate the mip chain
		//Copying down the whole mip chain doing a blit from mip-1 to mip
		Renderer::SubmitImmediateCommand([&](VkCommandBuffer commandBuffer) {
			for (uint32_t mip = 1; mip < m_MaxMipLevel; mip++) {
				for (uint32_t face = 0; face < m_CreateInfo.Layers; face++) {
					VkImageSubresourceLayers srcSubresource = VulkanAPI::ImageSubresourceLayers(m_CreateInfo.ImageType == ImageType::Type2DDepth || m_CreateInfo.ImageType == ImageType::Type2DArrayDepth
						? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT, mip - 1, face, 1);
					VkImageSubresourceLayers dstSubresource = VulkanAPI::ImageSubresourceLayers(srcSubresource.aspectMask, mip, face, 1);

					VkOffset3D srcOffsets[2] = { {}, {} };
					srcOffsets[1].x = (m_CreateInfo.Width >> (mip - 1));
					srcOffsets[1].y = (m_CreateInfo.Height >> (mip - 1));
					srcOffsets[1].z = 1;

					VkOffset3D dstOffsets[2] = { {}, {} };
					dstOffsets[1].x = (m_CreateInfo.Width >> mip);
					dstOffsets[1].y = (m_CreateInfo.Height >> mip);
					dstOffsets[1].z = 1;

					VkImageBlit blit = VulkanAPI::ImageBlit(srcSubresource, srcOffsets, dstSubresource, dstOffsets);

					// Prepare current mip level as image blit destination
					TransitionImageLayout(commandBuffer, m_Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mip, face, 1, 1);

					// Blit from previous level
					vkCmdBlitImage(commandBuffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

					// Prepare current mip level as image blit source for next level
					TransitionImageLayout(commandBuffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, mip, face, 1, 1);
				}
			}
			// After the loop, all mip layers are in TRANSFER_SRC layout, so transition all to SHADER_READ
			TransitionImageLayout(commandBuffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 0, m_MaxMipLevel, m_CreateInfo.Layers);
		});
	}

	void VulkanImage::SetLayout(VkCommandBuffer commandBuffer, VkImageLayout newLayout, uint32_t baseMipLevel, uint32_t baseArrayLayer, uint32_t levelCount, uint32_t layerCount) {
		TransitionImageLayout(commandBuffer, m_Image, m_CurrentLayout, newLayout, baseMipLevel, baseArrayLayer, levelCount, layerCount);
	}

	void VulkanImage::SetLayoutImmediate(VkImageLayout newLayout) {
		TransitionImageLayoutImmediate(m_Image, newLayout);
	}

	void VulkanImage::SetLayoutImmediate(VkImageLayout newLayout, uint32_t baseMipLevel, uint32_t baseArrayLayer, uint32_t levelCount, uint32_t layerCount) {
		TransitionImageLayoutImmediate(m_Image, newLayout, baseMipLevel, baseArrayLayer, levelCount, layerCount);
	}

	void VulkanImage::TransitionImageLayoutImmediate(VkImage image, VkImageLayout newLayout, uint32_t baseMipLevel, uint32_t baseArrayLayer, uint32_t levelCount, uint32_t layerCount) {
		TransitionImageLayoutImmediate(image, m_CurrentLayout, newLayout, baseMipLevel, baseArrayLayer, levelCount, layerCount);
	}

	void VulkanImage::TransitionImageLayoutImmediate(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t baseMipLevel, uint32_t baseArrayLayer, uint32_t levelCount, uint32_t layerCount) {
		Renderer::SubmitImmediateCommand([&](VkCommandBuffer commandBuffer) {
			TransitionImageLayout(commandBuffer, image, oldLayout, newLayout, baseMipLevel, baseArrayLayer, levelCount, layerCount);
		});
	}

	/// This is for mipmapping. We dont submit this to the queue. It's just a vorlage
	void VulkanImage::TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t baseMipLevel, uint32_t baseArrayLayer, uint32_t levelCount, uint32_t layerCount) {
		ImageMemoryBarrierCreateInfo createInfo;
		createInfo.ImageHandle = image;

		VkImageSubresourceRange subresourceRange = VulkanAPI::ImageSubresourceRange(m_CreateInfo.ImageType == ImageType::Type2DDepth || m_CreateInfo.ImageType == ImageType::Type2DArrayDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT,
																					baseMipLevel, baseArrayLayer, levelCount, layerCount);

		createInfo.SubResourceRange = subresourceRange;
		createInfo.OldLayout = oldLayout;
		createInfo.NewLayout = newLayout;

		ImageMemoryBarrier barrier(createInfo);
		barrier.RunBarrier(commandBuffer);

		m_CurrentLayout = newLayout;
	}

	void VulkanImage::RTCreateVulkanImageViewHandle(const Ref<VulkanRenderDevice>& vulkanDevice) {
		RTCreateVulkanImageViewHandle(vulkanDevice, m_ImageView, m_Image);

		if (m_CreateInfo.ImGuiUsage) {
			if (!m_ImGuiID) {
				Renderer::EnqueueToRenderThread([&]([[maybe_unused]] const Ref<RenderDevice>& device) {
					m_ImGuiID = ImGui_ImplVulkan_AddTexture(m_ImageView.GetSampler(), m_ImageView.GetVulkanHandle(), m_CurrentLayout);
				});
			} else {
				ImGui_ImplVulkanH_UpdateTexture((VkDescriptorSet)m_ImGuiID, m_ImageView.GetSampler(), m_ImageView.GetVulkanHandle(), m_CurrentLayout);
			}
		}
	}

	void VulkanImage::RTCreateVulkanImageViewHandle(const Ref<VulkanRenderDevice>& vulkanDevice, VulkanImageView& imageView, VkImage image) {
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

		ImageViewCreateInfo imageViewCreateInfo{
			.Image = image,
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
			.ModeW = GetImageAddressMode(m_CreateInfo.Parameter.W),
		};

		imageView = VulkanImageView(imageViewCreateInfo, vulkanDevice);
	}

	VulkanImageView::VulkanImageView(const ImageViewCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device)
		: m_CreateInfo(createInfo), m_VulkanDevice(device) {
		RTCreateView();
		if (m_CreateInfo.GenerateSampler)
			RTCreateSampler();
	}

	void VulkanImageView::RTCreateView() {
		auto GetImageType = [](ImageType type) {
			switch (type) {
				case ImageType::Type2DDepth:
				case ImageType::Type2DColor:
					return VK_IMAGE_VIEW_TYPE_2D;
				case ImageType::Type2DArrayDepth:
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
				case ImageType::Type2DArrayDepth:
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
		
		VkImageSubresourceRange subresourceRange = VulkanAPI::ImageSubresourceRange(m_CreateInfo.ImageType == ImageType::Type2DDepth || m_CreateInfo.ImageType == ImageType::Type2DArrayDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT,
																					0, 0, m_CreateInfo.MipmapLevel, GetLayerCount(m_CreateInfo.ImageType));
		VkComponentMapping components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
		VkImageViewCreateInfo createInfo = VulkanAPI::ImageViewCreateInfo(m_CreateInfo.Image, GetImageType(m_CreateInfo.ImageType), m_CreateInfo.Format, subresourceRange, components);

		LUCY_VK_ASSERT(vkCreateImageView(m_VulkanDevice->GetLogicalDevice(), &createInfo, nullptr, &m_ImageView));
	}

	void VulkanImageView::RTCreateSampler() {
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(m_VulkanDevice->GetPhysicalDevice(), &properties);

		VkSamplerCreateInfo createInfo = VulkanAPI::SamplerCreateInfo(m_CreateInfo.MagFilter, m_CreateInfo.MinFilter, m_CreateInfo.ModeU, m_CreateInfo.ModeV, m_CreateInfo.ModeW, VK_TRUE,
																	  properties.limits.maxSamplerAnisotropy, VK_BORDER_COLOR_INT_OPAQUE_BLACK, VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS, VK_SAMPLER_MIPMAP_MODE_LINEAR,
																	  0.0f, 0.0f, m_CreateInfo.GenerateMipmap ? (float)m_CreateInfo.MipmapLevel : 0.0f);
		LUCY_VK_ASSERT(vkCreateSampler(m_VulkanDevice->GetLogicalDevice(), &createInfo, nullptr, &m_Sampler));
	}

	void VulkanImageView::RTRecreate(const ImageViewCreateInfo& createInfo) {
		m_CreateInfo = createInfo;
		RTDestroyResource();
		RTCreateView();
		if (m_CreateInfo.GenerateSampler)
			RTCreateSampler();
	}

	void VulkanImageView::RTDestroyResource() {
		if (!m_ImageView || !m_Sampler)
			return;

		vkDestroyImageView(m_VulkanDevice->GetLogicalDevice(), m_ImageView, nullptr);
		if (m_CreateInfo.GenerateSampler)
			vkDestroySampler(m_VulkanDevice->GetLogicalDevice(), m_Sampler, nullptr);

		m_ImageView = VK_NULL_HANDLE;
		m_Sampler = VK_NULL_HANDLE;
	}

	uint32_t GetAPIImageFormat(ImageFormat format) {
		if (Renderer::GetRenderArchitecture() != RenderArchitecture::Vulkan) {
			LUCY_ASSERT(false);
			return VK_FORMAT_MAX_ENUM;
		}
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
			case ImageFormat::R16G16_SFLOAT:
				return VK_FORMAT_R16G16_SFLOAT;
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
			case ImageFormat::R32G32_SFLOAT:
				return VK_FORMAT_R32G32_SFLOAT;
			case ImageFormat::R32_SFLOAT:
				return VK_FORMAT_R32_SFLOAT;
			case ImageFormat::R32_UINT:
				return VK_FORMAT_R32_UINT;
			default:
				return VK_FORMAT_MAX_ENUM;
		}
	}

	ImageFormat GetLucyImageFormat(uint32_t format) {
		if (Renderer::GetRenderArchitecture() != RenderArchitecture::Vulkan) {
			LUCY_ASSERT(false);
			return ImageFormat::Unknown;
		}
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
			case VK_FORMAT_R16G16_SFLOAT:
				return ImageFormat::R16G16_SFLOAT;
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
			case VK_FORMAT_R32G32_SFLOAT:
				return ImageFormat::R32G32_SFLOAT;
			case VK_FORMAT_R32_SFLOAT:
				return ImageFormat::R32_SFLOAT;
			case VK_FORMAT_R32_UINT:
				return ImageFormat::R32_UINT;
			default:
				return ImageFormat::Unknown;
		}
	}
}