#include "lypch.h"

#include "Texture.h"
#include "OpenGLTexture2D.h"

#include "../Renderer.h"

namespace Lucy {

	RefLucy<Texture2D> Texture2D::Create(TextureSpecification& specs) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return CreateRef<OpenGLTexture2D>(specs);
				break;
			default:
				LUCY_CRITICAL("Other API's not supported!");
				LUCY_ASSERT(false);
				break;
		}
		return nullptr;
	}

	Texture2D::Texture2D(TextureSpecification& specs)
		: m_Specs(specs), m_Width(specs.Width), m_Height(specs.Height) {
	}
}
