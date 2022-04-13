#include "lypch.h"

#include "Texture.h"
#include "OpenGLTexture2D.h"
#include "VulkanImage.h"

#include "../Renderer.h"

namespace Lucy {

	RefLucy<Texture2D> Texture2D::Create(TextureSpecification& specs) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return CreateRef<OpenGLTexture2D>(specs);
				break;
			case RenderArchitecture::Vulkan:
				return CreateRef<VulkanImage2D>(specs);
				break;
			default:
				LUCY_ASSERT(false);
		}
		return nullptr;
	}

	Texture2D::Texture2D(TextureSpecification& specs)
		: m_Specs(specs), m_Width(specs.Width), m_Height(specs.Height) {
	}
}
