#include "VertexBuffer.h"

#include "OpenGL/OpenGLVertexBuffer.h"

namespace Lucy {

	VertexBuffer::VertexBuffer(uint32_t size, void* data)
		: Buffer(size)
	{
		m_Data = data;
	}

	void VertexBuffer::SetData(void* data)
	{
		m_Data = data;
	}

	RefLucy<VertexBuffer> VertexBuffer::Create(uint32_t size, void* data)
	{
		switch (Renderer::GetCurrentRenderContextType()) {
			case RenderContextType::OpenGL:
				return CreateRef<OpenGLVertexBuffer>(size, data);
				break;
			case RenderContextType::Vulkan:
				LUCY_CRITICAL("Vulkan not supported");
				LUCY_ASSERT(false);
				break;
		}
	}
}

