#include "lypch.h"
#include "VulkanImage2D.h"

#include "Renderer/Renderer.h"
#include "Renderer/Device/VulkanRenderDevice.h"

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
		allocator.CreateVulkanBufferVma(VulkanBufferUsage::CPUOnly, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, false, imageStagingBuffer, imageStagingBufferVma);

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
			SetLayoutImmediate(GetInitialImageLayout());

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
			SetLayoutImmediate(GetInitialImageLayout());

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
			SetLayoutImmediate(GetInitialImageLayout());

		RTCreateSampler(vulkanDevice);
		RTCreateVulkanImageViewHandle(vulkanDevice);
	}

	void VulkanImage2D::RTDestroyResource(RenderDevice* device) {
		if (!m_Image)
			return;

		//if (m_CreateInfo.ImGuiUsage)
			//ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)m_ImGuiID);

		m_ImageView.RTDestroyResource();
		device->RTDestroyResource(m_SamplerHandle);
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

				if (oldSamplerHandle)
					vulkanDevice->RTDestroyResource(oldSamplerHandle);

				if (oldImage)
					vulkanDevice->GetAllocator().DestroyImage(oldImage, oldImageVma);
			};
		});
	}
}