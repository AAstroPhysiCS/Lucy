#include "lypch.h"
#include "VulkanImage2D.h"

#include "Renderer/Renderer.h"
#include "Renderer/Device/VulkanRenderDevice.h"

#include "DDSHelper.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

namespace Lucy {

	VulkanImage2D::VulkanImage2D(const std::filesystem::path& path, const ImageCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device, std::string_view debugName)
		: VulkanImage(path, createInfo, debugName) {
		if (m_CreateInfo.ImageType != ImageType::Type2D)
			LUCY_ASSERT(false);

		RTCreateFromPath(device);

#if LUCY_DEBUG
		AddLabel(m_Image, device);
#endif
	}

	VulkanImage2D::VulkanImage2D(const ImageCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device, std::string_view debugName)
		: VulkanImage(createInfo, debugName) {
		if (m_CreateInfo.ImageType != ImageType::Type2D)
			LUCY_ASSERT(false);

		if (m_CreateInfo.ImageUsage == ImageUsage::AsDepthAttachment)
			RTCreateDepthImage(device);
		else
			RTCreateEmptyImage(device);

#if LUCY_DEBUG
		AddLabel(m_Image, device);
#endif
	}

	VulkanImage2D::VulkanImage2D(const Ref<VulkanImage2D>& other, const Ref<VulkanRenderDevice>& device)
		: VulkanImage(other->m_CreateInfo, "Copied VulkanImage2D") {
		m_CreateInfo = other->m_CreateInfo;
		m_Path = other->m_Path;
		m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		m_ImGuiID = 0;

		if (m_CreateInfo.ImageType != ImageType::Type2D)
			LUCY_ASSERT(false);

		if (!m_Path.empty())
			RTCreateFromPath(device);
		else if (m_CreateInfo.ImageUsage == ImageUsage::AsDepthAttachment)
			RTCreateDepthImage(device);
		else
			RTCreateEmptyImage(device);

#if LUCY_DEBUG
		AddLabel(m_Image, device);
#endif
	}

	void VulkanImage2D::RTCreateFromPath(const Ref<VulkanRenderDevice>& vulkanDevice) {
		if (DDS::IsDDS(m_Path)) {
			RTCreateFromDDS(vulkanDevice);
			return;
		}

		std::string pathInString = m_Path.string();
		uint8_t* data = nullptr;
		bool isHDR = stbi_is_hdr(pathInString.c_str());

		if (isHDR)
			data = (uint8_t*)stbi_loadf(pathInString.c_str(), (int32_t*)&m_CreateInfo.Width, (int32_t*)&m_CreateInfo.Height, &m_Channels, STBI_rgb_alpha);
		else
			data = stbi_load(pathInString.c_str(), (int32_t*)&m_CreateInfo.Width, (int32_t*)&m_CreateInfo.Height, &m_Channels, STBI_rgb_alpha);
		
		CalculateMaxMipLevel();

		LUCY_ASSERT(data != nullptr, "Failed to load a texture. Texture path: {0}", pathInString);
		LUCY_ASSERT(m_CreateInfo.Width > 0 && m_CreateInfo.Height > 0, "Width or height of the image is less than zero.");

		VkDeviceSize imageSize = (VkDeviceSize)m_CreateInfo.Width * m_CreateInfo.Height * 4 * GetFormatSize(m_CreateInfo.Format);

		VkBuffer imageStagingBuffer = VK_NULL_HANDLE;
		VmaAllocation imageStagingBufferVma = VK_NULL_HANDLE;

		VulkanAllocator& allocator = vulkanDevice->GetAllocator();
		allocator.CreateVulkanBufferVma(MemoryUsage::CPUOnly, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, false, imageStagingBuffer, imageStagingBufferVma);

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
			SetLayoutImmediate(GetPreferredLayout());

		allocator.DestroyBuffer(imageStagingBuffer, imageStagingBufferVma);

		RTCreateSampler(vulkanDevice);
		RTCreateVulkanImageViewHandle(vulkanDevice);
	}

	void VulkanImage2D::RTCreateEmptyImage(const Ref<VulkanRenderDevice>& vulkanDevice) {
		LUCY_ASSERT(m_CreateInfo.Width > 0 && m_CreateInfo.Height > 0, "Width or height of the image is less than zero.");

		CalculateMaxMipLevel();

		VkImageUsageFlags flags = GetImageFlagsBasedOnUsage();

		VulkanAllocator& allocator = vulkanDevice->GetAllocator();
		allocator.CreateVulkanImageVma(m_CreateInfo.Width, m_CreateInfo.Height, m_MaxMipLevel, (VkFormat)GetAPIImageFormat(m_CreateInfo.Format), m_CurrentLayout,
									   flags, VK_IMAGE_TYPE_2D, m_Image, m_ImageVma, 0, m_CreateInfo.Layers);

		if (m_CreateInfo.GenerateMipmap)
			GenerateMipmapsImmediate();
		else
			SetLayoutImmediate(GetPreferredLayout());

		RTCreateSampler(vulkanDevice);
		RTCreateVulkanImageViewHandle(vulkanDevice);
	}

	void VulkanImage2D::RTCreateDepthImage(const Ref<VulkanRenderDevice>& vulkanDevice) {
		LUCY_ASSERT(m_CreateInfo.Width > 0 && m_CreateInfo.Height > 0, "Width or height of the image is less than zero.");

		CalculateMaxMipLevel();
		
		//do the flags
		VkImageUsageFlags flags = GetImageFlagsBasedOnUsage();

		VulkanAllocator& allocator = vulkanDevice->GetAllocator();
		allocator.CreateVulkanImageVma(m_CreateInfo.Width, m_CreateInfo.Height, 1, (VkFormat)GetAPIImageFormat(m_CreateInfo.Format), m_CurrentLayout, 
			flags, VK_IMAGE_TYPE_2D, m_Image, m_ImageVma, 0U, m_CreateInfo.Layers);

		if (m_CreateInfo.GenerateMipmap)
			GenerateMipmapsImmediate();
		else
			SetLayoutImmediate(GetPreferredLayout());

		RTCreateSampler(vulkanDevice);
		RTCreateVulkanImageViewHandle(vulkanDevice);
	}

	void VulkanImage2D::RTCreateFromDDS(const Ref<VulkanRenderDevice>& device) {
		const auto& vulkanDevice = device->As<VulkanRenderDevice>();
		const auto& pathString = m_Path.string();

		std::ifstream file(m_Path, std::ios::binary | std::ios::ate);
		LUCY_ASSERT(file.is_open(), "Failed to open DDS texture: {0}", pathString);

		std::streamsize fileSize = file.tellg();
		file.seekg(0, std::ios::beg);

		uint32_t magic = 0;
		file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
		LUCY_ASSERT(magic == DDS::MAGIC, "Invalid DDS magic: {0}", pathString);

		DDS::Header header{};
		file.read(reinterpret_cast<char*>(&header), sizeof(header));

		LUCY_ASSERT(header.Size == sizeof(DDS::Header), "Invalid DDS header size: {0}", pathString);
		LUCY_ASSERT(header.PixelFormat.Size == sizeof(DDS::PixelFormat), "Invalid DDS pixel format header: {0}", pathString);
		LUCY_ASSERT(header.Width > 0 && header.Height > 0, "Invalid DDS dimensions: {0}", pathString);
		LUCY_ASSERT(!(header.Caps2 & DDS::DDSCAPS2_CUBEMAP), "VulkanImage2D does not support DDS cubemaps: {0}", pathString);
		LUCY_ASSERT(!(header.Caps2 & DDS::DDSCAPS2_VOLUME), "VulkanImage2D does not support DDS volume textures: {0}", pathString);

		DDS::HeaderDX10 headerDX10{};
		DDS::HeaderDX10* headerDX10Ptr = nullptr;

		bool hasDX10Header = header.PixelFormat.FourCC == DDS::MakeFourCC('D', 'X', '1', '0');
		if (hasDX10Header) {
			file.read(reinterpret_cast<char*>(&headerDX10), sizeof(headerDX10));

			LUCY_ASSERT(headerDX10.ResourceDimension == DDS::DDS_DIMENSION_TEXTURE2D, "DDS texture is not a Texture2D: {0}", pathString);
			LUCY_ASSERT(headerDX10.ArraySize == 1, "DDS texture arrays are not supported by VulkanImage2D: {0}", pathString);
			LUCY_ASSERT(!(headerDX10.MiscFlag & DDS::DDS_RESOURCE_MISC_TEXTURECUBE), "DDS cubemaps are not supported by VulkanImage2D: {0}", pathString);

			headerDX10Ptr = &headerDX10;
		}

		auto formatInfo = DDS::GetFormatInfo(header, headerDX10Ptr);
		LUCY_ASSERT(formatInfo.Format != ImageFormat::Unknown, "Unsupported DDS format. Only BC-compressed DDS textures are currently supported: {0}", pathString);

		//TODO: maybe support imgui usage?
		m_CreateInfo.ImGuiUsage = false;
		m_CreateInfo.Width = header.Width;
		m_CreateInfo.Height = header.Height;
		m_CreateInfo.Layers = 1;
		m_CreateInfo.Format = formatInfo.Format;

		m_MaxMipLevel = std::max(1U, header.MipMapCount);

		std::vector<VkBufferImageCopy> copyRegions;
		copyRegions.reserve(m_MaxMipLevel);

		VkDeviceSize imageSize = 0;

		uint32_t mipWidth = m_CreateInfo.Width;
		uint32_t mipHeight = m_CreateInfo.Height;

		for (uint32_t mipLevel = 0; mipLevel < m_MaxMipLevel; mipLevel++) {
			VkBufferImageCopy copyRegion{};
			copyRegion.bufferOffset = imageSize;
			copyRegion.bufferRowLength = 0;
			copyRegion.bufferImageHeight = 0;

			copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.imageSubresource.mipLevel = mipLevel;
			copyRegion.imageSubresource.baseArrayLayer = 0;
			copyRegion.imageSubresource.layerCount = 1;

			copyRegion.imageOffset = { 0, 0, 0 };
			copyRegion.imageExtent = { mipWidth, mipHeight, 1 };

			copyRegions.emplace_back(copyRegion);

			imageSize += DDS::GetMipSize(mipWidth, mipHeight, formatInfo.BlockSize);

			mipWidth = std::max(1U, mipWidth >> 1);
			mipHeight = std::max(1U, mipHeight >> 1);
		}

		std::streamoff dataOffset = file.tellg();
		LUCY_ASSERT(dataOffset >= 0 && static_cast<VkDeviceSize>(fileSize - dataOffset) >= imageSize, "DDS texture data is truncated: {0}", m_Path.string());

		VulkanAllocator& allocator = vulkanDevice->GetAllocator();

		VkBuffer imageStagingBuffer = VK_NULL_HANDLE;
		VmaAllocation imageStagingBufferVma = VK_NULL_HANDLE;
		allocator.CreateVulkanBufferVma(MemoryUsage::CPUOnly, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, false, imageStagingBuffer, imageStagingBufferVma);

		void* pixelData = nullptr;
		allocator.MapMemory(imageStagingBufferVma, pixelData);
		file.read(reinterpret_cast<char*>(pixelData), static_cast<std::streamsize>(imageSize));
		LUCY_ASSERT(file.good() || file.eof(), "Failed to read DDS texture data: {0}", m_Path.string());
		allocator.UnmapMemory(imageStagingBufferVma);
		file.close();

		VkImageUsageFlags flags = GetImageFlagsBasedOnUsage();

		allocator.CreateVulkanImageVma(m_CreateInfo.Width, m_CreateInfo.Height, m_MaxMipLevel, (VkFormat)GetAPIImageFormat(m_CreateInfo.Format), m_CurrentLayout, flags, VK_IMAGE_TYPE_2D, m_Image, m_ImageVma);

		TransitionImageLayoutImmediate(m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		CopyBufferToImageImmediate(imageStagingBuffer, copyRegions);
		SetLayoutImmediate(GetPreferredLayout());

		allocator.DestroyBuffer(imageStagingBuffer, imageStagingBufferVma);

		RTCreateSampler(vulkanDevice);
		RTCreateVulkanImageViewHandle(vulkanDevice);
	}

	void VulkanImage2D::RTDestroyResource(RenderDevice* device) {
		if (!m_Image)
			return;

		//if (m_CreateInfo.ImGuiUsage)
			//ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)m_ImGuiID);

		m_ImageView.RTDestroyResource();
		//device->RTDestroyResource(m_SamplerHandle);
		auto vulkanDevice = reinterpret_cast<VulkanRenderDevice*>(device);

		VulkanAllocator& allocator = vulkanDevice->GetAllocator();
		allocator.DestroyImage(m_Image, m_ImageVma);
		m_Image = VK_NULL_HANDLE;
	}

	void VulkanImage2D::RTRecreate(uint32_t width, uint32_t height) {
		m_CreateInfo.Width = width;
		m_CreateInfo.Height = height;

		Renderer::EnqueueResourceRecreate([this](const Ref<RenderDevice>& device) -> RenderDeletionFunc {
			auto vulkanDevice = device->As<VulkanRenderDevice>();

			VkImage oldImage = std::exchange(m_Image, VK_NULL_HANDLE);
			VmaAllocation oldImageVma = std::exchange(m_ImageVma, VK_NULL_HANDLE);
			VkImageView oldImageView = std::exchange(m_ImageView.m_ImageView, VK_NULL_HANDLE);

			auto oldSamplerHandle = std::exchange(m_SamplerHandle, {});

			m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			if (!m_Path.empty())
				RTCreateFromPath(vulkanDevice);
			else if (m_CreateInfo.ImageUsage == ImageUsage::AsDepthAttachment)
				RTCreateDepthImage(vulkanDevice);
			else
				RTCreateEmptyImage(vulkanDevice);

			return [oldImage, oldImageVma, oldImageView, oldSamplerHandle](const Ref<RenderDevice>& device) mutable {
				auto vulkanDevice = device->As<VulkanRenderDevice>();

				if (oldImageView)
					vkDestroyImageView(vulkanDevice->GetLogicalDevice(), oldImageView, nullptr);

				//if (oldSamplerHandle)
					//vulkanDevice->RTDestroyResource(oldSamplerHandle);

				if (oldImage)
					vulkanDevice->GetAllocator().DestroyImage(oldImage, oldImageVma);
			};
		});
	}
}