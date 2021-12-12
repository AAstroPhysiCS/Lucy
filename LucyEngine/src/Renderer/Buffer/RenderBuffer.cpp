#include "lypch.h"
#include "RenderBuffer.h"

#include "../Renderer.h"
#include "OpenGL/OpenGLRenderBuffer.h"

namespace Lucy {

	RenderBuffer::RenderBuffer(RenderBufferSpecification& specs) {
		m_Specs = specs;
	}

	RefLucy<RenderBuffer> RenderBuffer::Create(RenderBufferSpecification& specs) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
		case RenderArchitecture::OpenGL:
			return CreateRef<OpenGLRenderBuffer>(specs);
			break;
		}
	}
}