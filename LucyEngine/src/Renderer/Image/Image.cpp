#include "lypch.h"

#include "Image.h"
#include "OpenGLImage.h"
#include "VulkanImage.h"

#include "../Renderer.h"

namespace Lucy {

	Ref<Image2D> Image2D::Create(const std::string& path, ImageCreateInfo& createInfo) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return Memory::CreateRef<OpenGLImage2D>(path, createInfo);
				break;
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanImage2D>(path, createInfo);
				break;
			default:
				LUCY_ASSERT(false);
		}
		return nullptr;
	}
	
	Ref<Image2D> Image2D::Create(ImageCreateInfo& createInfo) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return Memory::CreateRef<OpenGLImage2D>(createInfo);
				break;
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanImage2D>(createInfo);
				break;
			default:
				LUCY_ASSERT(false);
		}
		return nullptr;
	}

	Image2D::Image2D(ImageCreateInfo& createInfo)
		: m_CreateInfo(createInfo), m_Width(createInfo.Width), m_Height(createInfo.Height) {
	}

	Image2D::Image2D(const std::string& path, ImageCreateInfo& createInfo)
		: m_Path(path), m_CreateInfo(createInfo) {
	}
}
