#pragma once

#include "Image.h"
#include "vulkan/vulkan.h"
#include "vma/vk_mem_alloc.h"

namespace Lucy {

	struct ImageViewCreateInfo {
		VkImage Image;
		ImageType ImageType;
		VkFormat Format;
		bool GenerateSampler = false;
		bool GenerateMipmap = false;
		uint32_t MipmapLevel = 1;
		uint32_t Layers = 1;
		VkFilter MagFilter;
		VkFilter MinFilter;
		VkSamplerAddressMode ModeU;
		VkSamplerAddressMode ModeV;
		VkSamplerAddressMode ModeW;
	};

	class VulkanImageView {
	public:
		VulkanImageView(const ImageViewCreateInfo& createInfo);
		~VulkanImageView() = default;

		inline VkImageView GetVulkanHandle() const { return m_ImageView; }
		inline VkSampler GetSampler() const { return m_Sampler; }

		void Recreate(const ImageViewCreateInfo& createInfo);
		void Destroy();
	private:
		VulkanImageView() = default; //so that we can initialize it as member

		void CreateView();
		void CreateSampler();

		VkImageView m_ImageView = VK_NULL_HANDLE;
		VkSampler m_Sampler = VK_NULL_HANDLE;
		ImageViewCreateInfo m_CreateInfo;

		friend class VulkanImage;
	};

	class VulkanImage : public Image {
	public:
		//Loads an asset
		VulkanImage(const std::string& path, ImageCreateInfo& createInfo);
		//Creates an empty image
		VulkanImage(ImageCreateInfo& createInfo);
		virtual ~VulkanImage() = default;

		inline VkImageLayout GetCurrentLayout() const { return m_CurrentLayout; }
		inline VkImage GetVulkanHandle() const { return m_Image; }
		inline const VulkanImageView& GetImageView() const { return m_ImageView; }

		void SetLayout(VkImageLayout newLayout);
		void CopyImageToBuffer(const VkBuffer& bufferToCopy, uint32_t layerCount = 1);
		void CopyBufferToImage(const VkBuffer& bufferToCopy, uint32_t layerCount = 1);

		void CopyImageToBuffer(const VkBuffer& bufferToCopy, const std::vector<VkBufferImageCopy>& imageCopyRegions);
		void CopyBufferToImage(const VkBuffer& bufferToCopy, const std::vector<VkBufferImageCopy>& bufferCopyRegions);
	protected:
		void CopyImageToBuffer(VkImage image, const VkBuffer& bufferToCopy, uint32_t layerCount = 1);
		void CopyBufferToImage(VkImage image, const VkBuffer& bufferToCopy, uint32_t layerCount = 1);

		void CopyImageToBuffer(VkImage image, const VkBuffer& bufferToCopy, const std::vector<VkBufferImageCopy>& imageCopyRegions);
		void CopyBufferToImage(VkImage image, const VkBuffer& bufferToCopy, const std::vector<VkBufferImageCopy>& bufferCopyRegions);

		void CreateVulkanImageViewHandle();

		void GenerateMipmaps();
		void GenerateMipmaps(VkImage image);

		void TransitionImageLayout(VkImage image, VkImageLayout newLayout, uint32_t baseMipLevel = 0, uint32_t levelCount = 1, uint32_t layerCount = 1);
		void TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t baseMipLevel = 0, uint32_t levelCount = 1, uint32_t layerCount = 1);
		void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t baseMipLevel = 0, uint32_t levelCount = 1, uint32_t layerCount = 1);

		VkImage m_Image = VK_NULL_HANDLE;
		VmaAllocation m_ImageVma = VK_NULL_HANDLE;
		VkImageLayout m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VulkanImageView m_ImageView;
	};

	static auto GetFormatSize = [](ImageFormat format) {
		switch (format) {
			case ImageFormat::R8G8B8A8_UNORM:
			case ImageFormat::R8G8B8A8_SRGB:
			case ImageFormat::R8G8B8A8_UINT:
			case ImageFormat::B8G8R8A8_SRGB:
			case ImageFormat::B8G8R8A8_UINT:
			case ImageFormat::B8G8R8A8_UNORM:
				return 1;
			case ImageFormat::R16G16B16A16_SFLOAT:
			case ImageFormat::R16G16B16A16_UINT:
			case ImageFormat::R16G16B16A16_UNORM:
				return 2;
			case ImageFormat::R32G32B32_SFLOAT:
				return 3;
			case ImageFormat::R32G32B32A32_SFLOAT:
			case ImageFormat::R32G32B32A32_UINT:
				return 4;
			default:
				LUCY_ASSERT(false);
				return 0;
		}
	};
}