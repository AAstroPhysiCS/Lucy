#include "lypch.h"
#include "VulkanImageCube.h"

#include "Renderer/Renderer.h"
#include "Renderer/Device/VulkanRenderDevice.h"

#include "stb/stb_image.h"

namespace Lucy {

	VulkanImageCube::VulkanImageCube(const std::filesystem::path& path, const ImageCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device, std::string_view debugName)
		: VulkanImage(path, createInfo, debugName) {
		m_CreateInfo.Layers = 6;

		LUCY_ASSERT(m_CreateInfo.ImageType == ImageType::TypeCube);
		LUCY_ASSERT(stbi_is_hdr(m_Path.generic_string().c_str()), "The texture isnt HDR. Texture path: {0}", m_Path.generic_string().c_str());

		RTCreateFromPath(device);

#if LUCY_DEBUG
		AddLabel(m_Image, device);
#endif
	}

	VulkanImageCube::VulkanImageCube(const ImageCreateInfo& createInfo, const Ref<VulkanRenderDevice>& device, std::string_view debugName)
		: VulkanImage(createInfo, debugName) {
		m_CreateInfo.Layers = 6;
		LUCY_ASSERT(m_CreateInfo.ImageType == ImageType::TypeCube);
		RTCreateEmptyImage(device);

#if LUCY_DEBUG
		AddLabel(m_Image, device);
#endif
	}

	void VulkanImageCube::RTCreateFromPath(const Ref<VulkanRenderDevice>& vulkanDevice) {
		VkImageUsageFlags flags = GetImageFlagsBasedOnUsage();

		CalculateMaxMipLevel();

		VulkanAllocator& allocator = vulkanDevice->GetAllocator();
		allocator.CreateVulkanImageVma(m_CreateInfo.Width, m_CreateInfo.Height, m_MaxMipLevel, (VkFormat)GetAPIImageFormat(m_CreateInfo.Format), m_CurrentLayout,
									   flags, VK_IMAGE_TYPE_2D, m_Image, m_ImageVma, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, m_CreateInfo.Layers);

		if (m_CreateInfo.GenerateMipmap)
			GenerateMipmapsImmediate();
		else //transitioning only then, when we dont care about mipmapping. Mipmapping already transitions to the right layout
			SetLayoutImmediate(GetPreferredLayout(), 0, 0, 1, m_CreateInfo.Layers);

		RTCreateSampler(vulkanDevice);
		RTCreateVulkanImageViewHandle(vulkanDevice);
	}

	void VulkanImageCube::RTCreateEmptyImage(const Ref<VulkanRenderDevice>& vulkanDevice) {
		VkImageUsageFlags flags = GetImageFlagsBasedOnUsage();

		VulkanAllocator& allocator = vulkanDevice->GetAllocator();
		allocator.CreateVulkanImageVma(m_CreateInfo.Width, m_CreateInfo.Height, m_MaxMipLevel, (VkFormat)GetAPIImageFormat(m_CreateInfo.Format), m_CurrentLayout,
			flags, VK_IMAGE_TYPE_2D, m_Image, m_ImageVma, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, m_CreateInfo.Layers);
		
		SetLayoutImmediate(GetPreferredLayout(), 0, 0, m_MaxMipLevel, m_CreateInfo.Layers);

		RTCreateSampler(vulkanDevice);
		RTCreateVulkanImageViewHandle(vulkanDevice);
	}

	void VulkanImageCube::RTRecreate(uint32_t width, uint32_t height) {
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

	void VulkanImageCube::RTDestroyResource(RenderDevice* device) {
		if (!m_Image)
			return;

		//if (m_CreateInfo.ImGuiUsage)
		//	ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)m_ImGuiID);

		auto vulkanDevice = reinterpret_cast<VulkanRenderDevice*>(device);

		VulkanAllocator& allocator = vulkanDevice->GetAllocator();

		m_ImageView.RTDestroyResource();
		//vulkanDevice->RTDestroyResource(m_SamplerHandle);

		allocator.DestroyImage(m_Image, m_ImageVma);
		m_Image = VK_NULL_HANDLE;
	}
}