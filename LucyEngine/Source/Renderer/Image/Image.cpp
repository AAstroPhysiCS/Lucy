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
				LUCY_ASSERT(false, "No suitable API found to create the resource!");
		}
		return nullptr;
	}

	Ref<Image> Image::Create(const Ref<VulkanImage2D>& other) {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanImage2D>(other);
			default:
				LUCY_ASSERT(false, "No suitable API found to create the resource!");
		}
		return nullptr;
	}
	
	Ref<Image> Image::Create(ImageCreateInfo& createInfo) {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanImage2D>(createInfo);
			default:
				LUCY_ASSERT(false, "No suitable API found to create the resource!");
		}
		return nullptr;
	}

	Ref<Image> Image::CreateCube(const std::string& path, ImageCreateInfo& createInfo) {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanImageCube>(path, createInfo);
			default:
				LUCY_ASSERT(false, "No suitable API found to create the resource!");
		}
		return nullptr;
	}

	Ref<Image> Image::CreateCube(ImageCreateInfo& createInfo) {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanImageCube>(createInfo);
			default:
				LUCY_ASSERT(false, "No suitable API found to create the resource!");
		}
		return nullptr;
	}

	static Ref<Image> s_BlankCube = nullptr;

	Ref<Image>& Image::GetBlankCube() {
		return s_BlankCube;
	}

	Image::Image(ImageCreateInfo& createInfo)
		: m_CreateInfo(createInfo), m_Width(createInfo.Width), m_Height(createInfo.Height) {
		if (m_CreateInfo.GenerateMipmap)
			m_MaxMipLevel = (uint32_t)glm::floor(glm::log2(glm::max(m_Width, m_Height))) + 1u;
	}

	Image::Image(const std::string& path, ImageCreateInfo& createInfo)
		: m_Path(path), m_CreateInfo(createInfo) {
	}
}
