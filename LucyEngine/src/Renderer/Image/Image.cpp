#include "lypch.h"

#include "Image.h"
#include "OpenGLImage.h"
#include "VulkanImage.h"

#include "../Renderer.h"

namespace Lucy {

	RefLucy<Image2D> Image2D::Create(const std::string& path, ImageSpecification& specs) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return CreateRef<OpenGLImage2D>(path, specs);
				break;
			case RenderArchitecture::Vulkan:
				return CreateRef<VulkanImage2D>(path, specs);
				break;
			default:
				LUCY_ASSERT(false);
		}
		return nullptr;
	}
	
	RefLucy<Image2D> Image2D::Create(ImageSpecification& specs) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return CreateRef<OpenGLImage2D>(specs);
				break;
			case RenderArchitecture::Vulkan:
				return CreateRef<VulkanImage2D>(specs);
				break;
			default:
				LUCY_ASSERT(false);
		}
		return nullptr;
	}

	Image2D::Image2D(ImageSpecification& specs)
		: m_Specs(specs), m_Width(specs.Width), m_Height(specs.Height) {
	}

	Image2D::Image2D(const std::string& path, ImageSpecification& specs)
		: m_Path(path), m_Specs(specs) {
	}
}
