#pragma once

#include "Image.h"
#include "vulkan/vulkan.h"
#include "vma/vk_mem_alloc.h"

namespace Lucy {

	struct ImageViewCreateInfo {
		ImageTarget Target;
		VkImage Image;
		VkFormat Format;
		VkImageViewType ViewType;
		VkFilter MagFilter;
		VkFilter MinFilter;
		VkSamplerAddressMode ModeU;
		VkSamplerAddressMode ModeV;
		VkSamplerAddressMode ModeW;
		bool GenerateSampler = false;
		bool GenerateMipmap = false;
		uint32_t MipmapLevel = 1;
	};

	class VulkanImageView {
	public:
		VulkanImageView(const ImageViewCreateInfo& createInfo);
		~VulkanImageView() = default;

		inline VkImageView GetVulkanHandle() { return m_ImageView; }
		inline VkImageView GetVulkanHandle() const { return m_ImageView; }
		inline VkSampler GetSampler() const { return m_Sampler; }

		void Recreate(const ImageViewCreateInfo& createInfo);
		void Destroy();
	private:
		VulkanImageView() = default; //for VulkanImage2D, so that we can initialize it as member

		void CreateView();
		void CreateSampler();

		VkImageView m_ImageView = VK_NULL_HANDLE;
		VkSampler m_Sampler = VK_NULL_HANDLE;
		ImageViewCreateInfo m_CreateInfo;

		friend class VulkanImage2D;
	};

	class VulkanImage2D : public Image2D {
	public:
		VulkanImage2D(const std::string& path, ImageCreateInfo& createInfo);
		VulkanImage2D(ImageCreateInfo& createInfo);
		virtual ~VulkanImage2D() = default;

		void Destroy() final override;
		void Recreate(uint32_t width, uint32_t height);

		inline VulkanImageView& GetImageView() { return m_ImageView; }
		inline VkImage GetVulkanHandle() const { return m_Image; }
		inline VkImageLayout GetCurrentLayout() const { return m_CurrentLayout; }

		void SetLayout(VkImageLayout newLayout);
		void CopyImageToBuffer(const VkBuffer& bufferToCopy);
	private:
		void CreateFromPath();
		void CreateEmptyImage();
		void CreateDepthImage();

		void CreateVulkanImageViewHandle();

		//helper function
		void GenerateMipmaps(VkImage image);
		void CopyBufferToImage(const VkBuffer& bufferToCopy);

		void TransitionImageLayout(VkImage image, VkImageLayout newLayout, uint32_t baseMipLevel = 0, uint32_t levelCount = 1);
		void TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t baseMipLevel = 0, uint32_t levelCount = 1);
		void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t baseMipLevel = 0, uint32_t levelCount = 1);

		VkImage m_Image = VK_NULL_HANDLE;
		VmaAllocation m_ImageVma = VK_NULL_HANDLE;
		VulkanImageView m_ImageView;

		VkImageLayout m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		uint32_t m_MaxMipLevel = 1;
	};
}

