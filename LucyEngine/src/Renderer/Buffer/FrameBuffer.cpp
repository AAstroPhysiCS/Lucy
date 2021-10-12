#include "FrameBuffer.h"

#include "OpenGL/OpenGLFrameBuffer.h"

namespace Lucy {
	
	RefLucy<FrameBuffer> Lucy::FrameBuffer::Create(FrameBufferSpecification& specs)
	{
		switch (Renderer::GetCurrentContext()) {
		case RendererContext::OPENGL:
			return CreateRef<OpenGLFrameBuffer>(specs);
			break;
		}
	}

	FrameBuffer::FrameBuffer(FrameBufferSpecification& specs)
		: m_Specs(specs)
	{
	}

}
