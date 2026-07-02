#pragma once

#include "Image.h"
#include "vulkan/vulkan.h"
#include "vma/vk_mem_alloc.h"

#include "VulkanImageSampler.h"

namespace Lucy {

	struct ImageViewCreateInfo {
		VkImage Image;
		ImageType ImageType;
		ImageUsage ImageUsage;
		VkFormat Format;
		MipmapCreateInfo GenerateMipmap = MipmapCreateInfo::NoMipmap();
		uint32_t MipmapLevel = 1;
		uint32_t Layers = 1;
	};

	class VulkanImageView {
	public:
		VulkanImageView(const ImageViewCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device, std::string_view debugName);
		~VulkanImageView() = default;

		inline VkImageView GetVulkanHandle() const { return m_ImageView; }
		inline VkImageView GetMipViewVulkanHandle(size_t mip) const { return m_MipViews[mip]; }

		void RTRecreate(const ImageViewCreateInfo& createInfo);
		void RTDestroyResource();
	private:
		VulkanImageView() = default;

		void RTCreateView();

		void AddLabel();

		VkImageView m_ImageView = VK_NULL_HANDLE;
		std::vector<VkImageView> m_MipViews; //one view per mip level
		ImageViewCreateInfo m_CreateInfo;

		std::string m_DebugName = "Unknown ImageView";

		Ref<VulkanRenderDevice> m_VulkanDevice = nullptr;

		friend class VulkanImage;
		friend class VulkanImage2D;
		friend class VulkanImageCube; //for irradiance (can't really reinstantiate the class again, since that would invoke it endlessly)
	};

	class VulkanImage : public Image {
	public:
		//Loads an asset
		VulkanImage(const std::filesystem::path& path, const ImageCreateInfo& createInfo, std::string_view debugName);
		//Creates an empty image
		VulkanImage(const ImageCreateInfo& createInfo, std::string_view debugName);
		virtual ~VulkanImage() = default;

		virtual void RTRecreate(uint32_t width, uint32_t height) = 0;

		inline VkImageLayout GetCurrentLayout() const { return m_CurrentLayout; }
		inline VkImage GetVulkanHandle() const { return m_Image; }
		inline const VulkanImageView& GetImageView() const { return m_ImageView; }
		inline const VulkanImageSampler& GetSampler() const { return m_Sampler; }

		void CopyImageToImage(VkCommandBuffer commandBuffer, const Ref<VulkanImage>& destImage, const std::vector<VkImageCopy>& imageCopyRegions);
		void CopyImageToBufferImmediate(const VkBuffer& bufferToCopy, uint32_t layerCount = 1);
	protected:
		void AddLabel(VkImage image, const Ref<VulkanRenderDevice>& device);

		void SetLayout(VkImageLayout newLayout);
		void SetLayout(VkCommandBuffer commandBuffer, VkImageLayout newLayout, uint32_t baseMipLevel, uint32_t baseArrayLayer, uint32_t levelCount, uint32_t layerCount);

		void SetLayoutImmediate(VkImageLayout newLayout);
		void SetLayoutImmediate(VkImageLayout newLayout, uint32_t baseMipLevel, uint32_t baseArrayLayer, uint32_t levelCount, uint32_t layerCount);

		void CopyImageToImageImmediate(const Ref<VulkanImage>& destImage, const std::vector<VkImageCopy>& imageCopyRegions);
		void CopyImageToImageImmediate(const VulkanImage* destImage, const std::vector<VkImageCopy>& imageCopyRegions);
		void CopyImageToImageImmediate(VkImage image, VkImageLayout layout, const std::vector<VkImageCopy>& regions);

		void CopyImageToBufferImmediate(const VkBuffer& bufferToCopy, const std::vector<VkBufferImageCopy>& imageCopyRegions);
		void CopyImageToBufferImmediate(VkImage image, const VkBuffer& bufferToCopy, uint32_t layerCount = 1);
		void CopyImageToBufferImmediate(VkImage image, const VkBuffer& bufferToCopy, const std::vector<VkBufferImageCopy>& imageCopyRegions);

		void CopyBufferToImageImmediate(const VkBuffer& bufferToCopy, uint32_t layerCount = 1);
		void CopyBufferToImageImmediate(const VkBuffer& bufferToCopy, const std::vector<VkBufferImageCopy>& bufferCopyRegions);
		void CopyBufferToImageImmediate(VkImage image, const VkBuffer& bufferToCopy, uint32_t layerCount = 1);
		void CopyBufferToImageImmediate(VkImage image, const VkBuffer& bufferToCopy, const std::vector<VkBufferImageCopy>& bufferCopyRegions);

		void GenerateMipmapsImmediate();

		void TransitionImageLayoutImmediate(VkImage image, VkImageLayout newLayout, uint32_t baseMipLevel = 0, uint32_t baseArrayLayer = 0, uint32_t levelCount = 1, uint32_t layerCount = 1);
		void TransitionImageLayoutImmediate(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t baseMipLevel = 0, uint32_t baseArrayLayer = 0, uint32_t levelCount = 1, uint32_t layerCount = 1);
		void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t baseMipLevel = 0, uint32_t baseArrayLayer = 0, uint32_t levelCount = 1, uint32_t layerCount = 1);

		void RTCreateVulkanImageViewHandle(const Ref<VulkanRenderDevice>& device);
		void RTCreateVulkanImageViewHandle(const Ref<VulkanRenderDevice>& device, VulkanImageView& imageView, VkImage image);

		void RTCreateSampler(const Ref<VulkanRenderDevice>& device);

		VkImageUsageFlags GetImageFlagsBasedOnUsage();
		VkImageLayout GetInitialImageLayout();

		VkImage m_Image = VK_NULL_HANDLE;
		VmaAllocation m_ImageVma = VK_NULL_HANDLE;
		VkImageLayout m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VulkanImageView m_ImageView;
		VulkanImageSampler m_Sampler;

		friend class VulkanRenderer; //for SetImageLayout/Immediate
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