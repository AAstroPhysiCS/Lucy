#include "lypch.h"
#include "RenderBuffer.h"

#include "../Renderer.h"
#include "OpenGL/OpenGLRenderBuffer.h"

namespace Lucy {

	RenderBuffer::RenderBuffer(const RenderBufferSpecification& specs) :
		m_Specs(specs) {
	}

	RefLucy<RenderBuffer> RenderBuffer::Create(const RenderBufferSpecification& specs) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return CreateRef<OpenGLRenderBuffer>(specs);
				break;
		}
		return nullptr;
	}
}