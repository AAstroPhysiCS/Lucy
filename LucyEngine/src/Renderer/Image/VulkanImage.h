#pragma once

#include "Image.h"
#include "vulkan/vulkan.h"
#include "vma/vk_mem_alloc.h"

namespace Lucy {

	struct ImageViewSpecification {
		VkImage Image;
		VkFormat Format;
		VkImageViewType ViewType;
		VkFilter MagFilter;
		VkFilter MinFilter;
		VkSamplerAddressMode ModeU;
		VkSamplerAddressMode ModeV;
		VkSamplerAddressMode ModeW;
		bool GenerateMipmap = false;
		bool GenerateSampler = false;
		bool DepthEnable = false;
	};

	class VulkanImageView {
	public:
		VulkanImageView(const ImageViewSpecification& specs);
		~VulkanImageView() = default;

		inline VkImageView GetVulkanHandle() { return m_ImageView; }
		inline VkImageView GetVulkanHandle() const { return m_ImageView; }
		inline VkSampler GetSampler() const { return m_Sampler; }

		void Recreate(const ImageViewSpecification& specs);
		void Destroy();
	private:
		VulkanImageView() = default; //for VulkanImage2D, so that we can initialize it as member

		void CreateView();
		void CreateSampler();

		VkImageView m_ImageView = VK_NULL_HANDLE;
		VkSampler m_Sampler = VK_NULL_HANDLE;
		ImageViewSpecification m_Specs;

		friend class VulkanImage2D;
	};

	class VulkanImage2D : public Image2D {
	public:
		VulkanImage2D(const std::string& path, ImageSpecification& specs);
		VulkanImage2D(ImageSpecification& specs);
		virtual ~VulkanImage2D() = default;

		void Bind() override;
		void Unbind() override;
		void Destroy() override;
		void Recreate(uint32_t width, uint32_t height);

		inline VulkanImageView& GetImageView() { return m_ImageView; }
		inline VkImage GetVulkanHandle() const { return m_Image; }
		inline VkImageLayout GetCurrentLayout() const { return m_CurrentLayout; }
	private:
		void CreateFromPath();
		void CreateEmptyImage();
		void CreateDepthImage();

		void CreateVulkanImageViewHandle();

		//helper functions
		void CopyImage(const VkBuffer& imageStagingBuffer); //should maybe be public?
		void TransitionImageLayout(VkImage image, VkImageLayout newLayout);
		void DefineMasksByLayout(VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags& srcAccessMask, VkAccessFlags& destAccessMask,
								 VkPipelineStageFlags& sourceStage, VkPipelineStageFlags& destStage);

		VkImage m_Image = VK_NULL_HANDLE;
		VmaAllocation m_ImageVma = VK_NULL_HANDLE;
		VulkanImageView m_ImageView;

		VkImageLayout m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	};
}

