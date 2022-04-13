#pragma once

#include "Texture.h"
#include "vulkan/vulkan.h"
#include "vma/vk_mem_alloc.h"

namespace Lucy {

	struct ImageViewSpecification {
		VkImage Image;
		VkFormat Format;
		VkImageViewType ViewType;
	};

	class VulkanImageView {
	public:
		VulkanImageView(const ImageViewSpecification& specs);
		~VulkanImageView() = default;

		inline VkImageView GetVulkanHandle() { return m_ImageView; }
		inline VkImageView GetVulkanHandle() const { return m_ImageView; }

		void Destroy();
	private:
		void CreateView();
		void CreateSampler();

		VkImageView m_ImageView;
		VkSampler m_Sampler;
		ImageViewSpecification m_Specs;
	};

	class VulkanImage2D : public Texture2D {
	public:
		VulkanImage2D(TextureSpecification& specs);

		void Bind() override;
		void Unbind() override;
		void Destroy() override;

		inline VkImage GetVulkanHandle() const { return m_Image; }
	private:
		//helper functions
		void CopyImage(); //should maybe be public?
		void TransitionImageLayout(VkImage image, VkImageLayout newLayout);
		void DefineMasksByLayout(VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags& srcAccessMask, VkAccessFlags& destAccessMask,
								 VkPipelineStageFlags& sourceStage, VkPipelineStageFlags& destStage);
		void CreateImage();

		VkBuffer m_ImageStagingBuffer;
		VmaAllocation m_ImageStagingBufferVma;

		VkImage m_Image;
		VmaAllocation m_ImageVma;
		VkImageLayout m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	};
}

