#include "Texture.h"
#include "OpenGLTexture2D.h"

#include "../Renderer.h"

namespace Lucy {
	
	RefLucy<Texture2D> Texture2D::Create(TextureSpecification& specs)
	{
		switch (Renderer::GetCurrentRenderContextType()) {
			case RenderContextType::OPENGL:
				return CreateRef<OpenGLTexture2D>(specs);
				break;
		}
	}
	
	uint32_t Texture2D::GetID()
	{
		return m_Id;
	}

	Texture2D::Texture2D(TextureSpecification& specs)
		: m_Specs(specs)
	{
		m_Width = specs.width;
		m_Height = specs.height;
	}

}
