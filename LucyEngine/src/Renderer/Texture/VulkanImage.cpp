#include "lypch.h"
#include "VulkanImage.h"

#include "Renderer/VulkanRHI.h"
#include "../VulkanAllocator.h"
#include "../Context/VulkanDevice.h"

#include "stb/stb_image.h"

namespace Lucy {

	VulkanImage2D::VulkanImage2D(TextureSpecification& specs)
		: Texture2D(specs) {
		uint8_t* data = nullptr;
		if (specs.Path) {
			data = stbi_load(specs.Path, &m_Width, &m_Height, &m_Channels, 0);

			specs.Format.InternalFormat = specs.Format.Format;

			if (!data) LUCY_CRITICAL(fmt::format("Failed to load a texture. Texture path: {0}", specs.Path));
		} else {
			m_Width = specs.Width;
			m_Height = specs.Height;
		}

		if (m_Width == 0 && m_Height == 0) LUCY_ASSERT(false);

		int64_t imageSize = (int64_t)m_Width * m_Height * 4;

		VulkanAllocator& allocator = VulkanAllocator::Get();
		allocator.CreateVulkanBufferVMA(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO, m_ImageStagingBuffer, m_ImageStagingBufferVma);

		VulkanDevice& device = VulkanDevice::Get();

		void* pixelData;
		vmaMapMemory(allocator.GetVMAInstance(), m_ImageStagingBufferVma, &pixelData);
		//TODO: data could be 0, meaning we want a buffer that is empty. Figure it out!
		memcpy(pixelData, data, imageSize);
		vmaUnmapMemory(allocator.GetVMAInstance(), m_ImageStagingBufferVma);

		stbi_image_free(data);
		CreateImage();

		TransitionImageLayout(m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyImage();
		TransitionImageLayout(m_Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vmaDestroyBuffer(allocator.GetVMAInstance(), m_ImageStagingBuffer, m_ImageStagingBufferVma);
	}

	void VulkanImage2D::CreateImage() {
		VkImageCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.imageType = VK_IMAGE_TYPE_2D;
		info.extent.width = m_Width;
		info.extent.height = m_Height;
		info.extent.depth = 1;
		info.mipLevels = 1; //TODO: no mipmapping yet
		info.arrayLayers = 1;
		info.format = (VkFormat)m_Specs.Format.Format;
		info.tiling = VK_IMAGE_TILING_OPTIMAL;
		info.initialLayout = m_CurrentLayout;
		info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		info.samples = VK_SAMPLE_COUNT_1_BIT; //no multisampling

		VulkanDevice& device = VulkanDevice::Get();
		LUCY_VK_ASSERT(vkCreateImage(device.GetLogicalDevice(), &info, nullptr, &m_Image));
	}

	void VulkanImage2D::CopyImage() {
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
			region.imageExtent.width = (uint32_t)m_Specs.Width;
			region.imageExtent.height = (uint32_t)m_Specs.Height;
			region.imageExtent.depth = 1;

			vkCmdCopyBufferToImage(commandBuffer, m_ImageStagingBuffer, m_Image, m_CurrentLayout, 1, &region);
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
		VulkanAllocator& allocator = VulkanAllocator::Get();
		vmaDestroyImage(allocator.GetVMAInstance(), m_Image, m_ImageVma);
	}

	VulkanImageView::VulkanImageView(const ImageViewSpecification& specs)
		: m_Specs(specs) {
		CreateView();
		CreateSampler();
	}

	void VulkanImageView::CreateView() {
		VulkanDevice& device = VulkanDevice::Get();

		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = m_Specs.Image;
		createInfo.viewType = m_Specs.ViewType;
		createInfo.format = m_Specs.Format;
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
		VkSamplerCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	}

	void VulkanImageView::Destroy() {
		vkDestroyImageView(VulkanDevice::Get().GetLogicalDevice(), m_ImageView, nullptr);
	}
}