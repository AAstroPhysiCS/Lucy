#include "RenderBuffer.h"

#include "../Renderer.h"
#include "OpenGL/OpenGLRenderBuffer.h"

namespace Lucy {
	
	RenderBuffer::RenderBuffer(RenderBufferSpecification& specs)
	{
		m_Specs = specs;
	}

	RefLucy<RenderBuffer> RenderBuffer::Create(RenderBufferSpecification& specs)
	{
		switch (Renderer::GetCurrentRenderContextType()) {
			case RenderContextType::OpenGL:
				return CreateRef<OpenGLRenderBuffer>(specs);
				break;
		}
	}
}
