#include "lypch.h"
#include "VulkanImage.h"

#include "Renderer/VulkanRHI.h"
#include "Renderer/Renderer.h"

#include "../VulkanAllocator.h"
#include "../Context/VulkanDevice.h"

#include "stb/stb_image.h"
#include "../vendor/ImGui/imgui_impl_vulkan.h"

namespace Lucy {

	VulkanImage2D::VulkanImage2D(const std::string& path, ImageSpecification& specs)
		: Image2D(path, specs) {
		if (m_Specs.ImageType != ImageType::Type2D) LUCY_ASSERT(false);
		Renderer::Enqueue([=]() {
			CreateFromPath();
		});
	}

	VulkanImage2D::VulkanImage2D(ImageSpecification& specs)
		: Image2D(specs) {
		if (m_Specs.ImageType != ImageType::Type2D) LUCY_ASSERT(false);
		Renderer::Enqueue([=]() {
			RefLucy<VulkanRHIImageDesc> imageDesc = As(m_Specs.InternalInfo, VulkanRHIImageDesc);
			if (imageDesc->DepthEnable)
				CreateDepthImage();
			else
				CreateEmptyImage();
		});
	}

	void VulkanImage2D::CreateFromPath() {
		uint8_t* data = nullptr;
		data = stbi_load(m_Path.c_str(), &m_Width, &m_Height, &m_Channels, STBI_rgb_alpha);

		if (!data)
			LUCY_CRITICAL(fmt::format("Failed to load a texture. Texture path: {0}", m_Path));
		if (m_Width == 0 && m_Height == 0)
			LUCY_ASSERT(false);

		uint64_t imageSize = (uint64_t)m_Width * m_Height * 4;

		VkBuffer m_ImageStagingBuffer = VK_NULL_HANDLE;
		VmaAllocation m_ImageStagingBufferVma = VK_NULL_HANDLE;

		VulkanAllocator& allocator = VulkanAllocator::Get();
		allocator.CreateVulkanBufferVma(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO,
										m_ImageStagingBuffer, m_ImageStagingBufferVma);

		void* pixelData;
		vmaMapMemory(allocator.GetVmaInstance(), m_ImageStagingBufferVma, &pixelData);
		memcpy(pixelData, data, imageSize);
		vmaUnmapMemory(allocator.GetVmaInstance(), m_ImageStagingBufferVma);

		stbi_image_free(data);

		allocator.CreateVulkanImageVma(m_Width, m_Height, (VkFormat)m_Specs.Format, m_CurrentLayout, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
									   VK_IMAGE_TYPE_2D, VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO, m_Image, m_ImageVma);

		TransitionImageLayout(m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyImage(m_ImageStagingBuffer);
		TransitionImageLayout(m_Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vmaDestroyBuffer(allocator.GetVmaInstance(), m_ImageStagingBuffer, m_ImageStagingBufferVma);

		CreateVulkanImageViewHandle();
	}

	void VulkanImage2D::CreateEmptyImage() {
		if (m_Width == 0 && m_Height == 0) LUCY_ASSERT(false);

		VulkanAllocator& allocator = VulkanAllocator::Get();
		allocator.CreateVulkanImageVma(m_Width, m_Height, (VkFormat)m_Specs.Format, m_CurrentLayout, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
									   VK_IMAGE_TYPE_2D, VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO, m_Image, m_ImageVma);

		m_CurrentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		CreateVulkanImageViewHandle();
	}

	void VulkanImage2D::CreateDepthImage() {
		if (m_Width == 0 && m_Height == 0) LUCY_ASSERT(false);

		VulkanAllocator& allocator = VulkanAllocator::Get();
		allocator.CreateVulkanImageVma(m_Width, m_Height, (VkFormat)m_Specs.Format, m_CurrentLayout, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
									   VK_IMAGE_TYPE_2D, VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO, m_Image, m_ImageVma);

		m_CurrentLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		CreateVulkanImageViewHandle();
	}

	void VulkanImage2D::CreateVulkanImageViewHandle() {
		RefLucy<VulkanRHIImageDesc> imageDesc = As(m_Specs.InternalInfo, VulkanRHIImageDesc);

		ImageViewSpecification imageViewSpecification;
		imageViewSpecification.Format = (VkFormat)m_Specs.Format;
		imageViewSpecification.Image = m_Image;
		imageViewSpecification.ViewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewSpecification.GenerateMipmap = m_Specs.GenerateMipmap;
		imageViewSpecification.GenerateSampler = imageDesc->GenerateSampler;
		imageViewSpecification.MagFilter = (VkFilter)m_Specs.Parameter.Mag;
		imageViewSpecification.MinFilter = (VkFilter)m_Specs.Parameter.Min;
		imageViewSpecification.ModeU = (VkSamplerAddressMode)m_Specs.Parameter.U;
		imageViewSpecification.ModeV = (VkSamplerAddressMode)m_Specs.Parameter.V;
		imageViewSpecification.ModeW = (VkSamplerAddressMode)m_Specs.Parameter.W;
		imageViewSpecification.DepthEnable = imageDesc->DepthEnable;

		m_ImageView = VulkanImageView(imageViewSpecification);

		if (imageDesc->ImGuiUsage) {
			if (!m_ID) {
				Renderer::Enqueue([&]() {
					m_ID = ImGui_ImplVulkan_AddTexture(m_ImageView.GetSampler(), m_ImageView.GetVulkanHandle(), m_CurrentLayout);
				});
			} else {
				ImGui_ImplVulkanH_UpdateTexture((VkDescriptorSet) m_ID, m_ImageView.GetSampler(), m_ImageView.GetVulkanHandle(), m_CurrentLayout);
			}
		}
	}

	void VulkanImage2D::CopyImage(const VkBuffer& imageStagingBuffer) {
		VulkanRHI::RecordSingleTimeCommand([&](VkCommandBuffer commandBuffer) {
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

	void VulkanImage2D::TransitionImageLayout(VkImage image, VkImageLayout newLayout) {
		VulkanRHI::RecordSingleTimeCommand([&](VkCommandBuffer commandBuffer) {
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = m_CurrentLayout;
			barrier.newLayout = newLayout;
			barrier.image = image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; //TODO: Investigate and support queue ownership transfer
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

			VkPipelineStageFlags sourceStage, destStage;
			DefineMasksByLayout(barrier.oldLayout, barrier.newLayout, barrier.srcAccessMask, barrier.dstAccessMask, sourceStage, destStage);

			vkCmdPipelineBarrier(commandBuffer, sourceStage, destStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

			m_CurrentLayout = newLayout;
		});
	}

	void VulkanImage2D::DefineMasksByLayout(VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags& srcAccessMask, VkAccessFlags& destAccessMask,
											VkPipelineStageFlags& sourceStage, VkPipelineStageFlags& destStage) {
		switch (oldLayout) {
			case VK_IMAGE_LAYOUT_UNDEFINED:
				srcAccessMask = 0;
				sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				break;
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				break;
			default:
				LUCY_ASSERT(false);
		}

		switch (newLayout) {
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				destAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				destStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				break;
			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				destAccessMask = VK_ACCESS_SHADER_READ_BIT;
				destStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				break;
			default:
				LUCY_ASSERT(false);
		}
	}

	void VulkanImage2D::Bind() {

	}

	void VulkanImage2D::Unbind() {

	}

	void VulkanImage2D::Destroy() {
		if (!m_Image) return;
		m_ImageView.Destroy();
		VulkanAllocator& allocator = VulkanAllocator::Get();
		vmaDestroyImage(allocator.GetVmaInstance(), m_Image, m_ImageVma);
		m_Image = VK_NULL_HANDLE;
	}

	void VulkanImage2D::Recreate(uint32_t width, uint32_t height) {
		m_Width = width;
		m_Height = height;
		auto& desc = As(m_Specs.InternalInfo, VulkanRHIImageDesc);

		Destroy();
		m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		if (!m_Path.empty())
			CreateFromPath();
		else if (desc->DepthEnable)
			CreateDepthImage();
		else
			CreateEmptyImage();
	}

	VulkanImageView::VulkanImageView(const ImageViewSpecification& specs)
		: m_Specs(specs) {
		CreateView();
		if (m_Specs.GenerateSampler)
			CreateSampler();
	}

	void VulkanImageView::CreateView() {
		VulkanDevice& device = VulkanDevice::Get();

		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = m_Specs.Image;
		createInfo.viewType = m_Specs.ViewType;
		createInfo.format = m_Specs.Format;

		if (m_Specs.DepthEnable)
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		else
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		createInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };

		LUCY_VK_ASSERT(vkCreateImageView(device.GetLogicalDevice(), &createInfo, nullptr, &m_ImageView));
	}

	void VulkanImageView::CreateSampler() {
		VulkanDevice& device = VulkanDevice::Get();

		VkSamplerCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		createInfo.magFilter = m_Specs.MagFilter;
		createInfo.minFilter = m_Specs.MinFilter;
		createInfo.addressModeU = m_Specs.ModeU;
		createInfo.addressModeV = m_Specs.ModeV;
		createInfo.addressModeW = m_Specs.ModeW;

		createInfo.anisotropyEnable = VK_TRUE;
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(device.GetPhysicalDevice(), &properties);
		createInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy; //best quality, TODO: in the future, add an option
		createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		createInfo.unnormalizedCoordinates = VK_FALSE;
		//TODO: for pcf
		createInfo.compareEnable = VK_FALSE;
		if (!m_Specs.GenerateMipmap) {
			createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.mipLodBias = 0.0f;
			createInfo.minLod = 0.0f;
			createInfo.maxLod = 0.0f;
		} else {
			//TODO: do mipmapping
		}

		LUCY_VK_ASSERT(vkCreateSampler(device.GetLogicalDevice(), &createInfo, nullptr, &m_Sampler));
	}

	void VulkanImageView::Recreate(const ImageViewSpecification& specs) {
		m_Specs = specs;
		Destroy();
		CreateView();
		if (m_Specs.GenerateSampler)
			CreateSampler();
	}

	void VulkanImageView::Destroy() {
		vkDestroyImageView(VulkanDevice::Get().GetLogicalDevice(), m_ImageView, nullptr);
		if (m_Specs.GenerateSampler)
			vkDestroySampler(VulkanDevice::Get().GetLogicalDevice(), m_Sampler, nullptr);
	}
}