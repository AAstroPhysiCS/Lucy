#include "FrameBuffer.h"

#include "OpenGL/OpenGLFrameBuffer.h"
#include "../Renderer.h"

namespace Lucy {
	
	RefLucy<FrameBuffer> FrameBuffer::Create(FrameBufferSpecification& specs)
	{
		switch (Renderer::GetCurrentRenderContextType()) {
			case RenderContextType::OpenGL:
				return CreateRef<OpenGLFrameBuffer>(specs);
				break;
		}
	}

	FrameBuffer::FrameBuffer(FrameBufferSpecification& specs)
		: m_Specs(specs)
	{
	}
}
