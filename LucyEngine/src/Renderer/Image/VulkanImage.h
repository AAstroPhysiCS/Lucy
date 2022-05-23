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
		VulkanImage2D(ImageSpecification& specs);

		void Bind() override;
		void Unbind() override;
		void Destroy() override;
		void Recreate();

		inline VulkanImageView& GetImageView() { return m_ImageView; }
		inline VkImage GetVulkanHandle() const { return m_Image; }
		inline VkImageLayout GetCurrentLayout() const { return m_CurrentLayout; }
	private:
		//helper functions
		void CopyImage(); //should maybe be public?
		void TransitionImageLayout(VkImage image, VkImageLayout newLayout);
		void DefineMasksByLayout(VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags& srcAccessMask, VkAccessFlags& destAccessMask,
								 VkPipelineStageFlags& sourceStage, VkPipelineStageFlags& destStage);
		void CreateImage(VkImageUsageFlags usage);
		void Create();

		VkBuffer m_ImageStagingBuffer = VK_NULL_HANDLE;
		VmaAllocation m_ImageStagingBufferVma = VK_NULL_HANDLE;

		VkImage m_Image = VK_NULL_HANDLE;
		VmaAllocation m_ImageVma = VK_NULL_HANDLE;
		VkImageLayout m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VulkanImageView m_ImageView;
	};
}

