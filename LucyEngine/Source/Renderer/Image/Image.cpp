#include "lypch.h"
#include "Image.h"
#include "VulkanImage2D.h"
#include "VulkanImageCube.h"

#include "Core/Base.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	Ref<Image> Image::Create(const std::string& path, ImageCreateInfo& createInfo) {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanImage2D>(path, createInfo);
			default:
				LUCY_ASSERT(false);
		}
		return nullptr;
	}

	Ref<Image> Image::Create(const Ref<VulkanImage2D>& other) {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanImage2D>(other);
			default:
				LUCY_ASSERT(false);
		}
		return nullptr;
	}
	
	Ref<Image> Image::Create(ImageCreateInfo& createInfo) {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanImage2D>(createInfo);
			default:
				LUCY_ASSERT(false);
		}
		return nullptr;
	}

	Ref<Image> Image::CreateCube(const std::string& path, ImageCreateInfo& createInfo) {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanImageCube>(path, createInfo);
			default:
				LUCY_ASSERT(false);
		}
		return nullptr;
	}

	Ref<Image> Image::CreateCube(ImageCreateInfo& createInfo) {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanImageCube>(createInfo);
			default:
				LUCY_ASSERT(false);
		}
		return nullptr;
	}

	Image::Image(ImageCreateInfo& createInfo)
		: m_CreateInfo(createInfo), m_Width(createInfo.Width), m_Height(createInfo.Height) {
		if (m_CreateInfo.GenerateMipmap)
			m_MaxMipLevel = glm::floor(glm::log2(glm::max(m_Width, m_Height))) + 1;
	}

	Image::Image(const std::string& path, ImageCreateInfo& createInfo)
		: m_Path(path), m_CreateInfo(createInfo) {
	}
}
