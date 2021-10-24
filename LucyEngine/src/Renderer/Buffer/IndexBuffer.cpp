#include "IndexBuffer.h"

#include "OpenGL/OpenGLIndexBuffer.h"

namespace Lucy {
	
	RefLucy<IndexBuffer> IndexBuffer::Create(uint32_t size, void* data)
	{
		switch (Renderer::GetCurrentRenderContextType()) {
			case RenderContextType::OpenGL:
				return CreateRef<OpenGLIndexBuffer>(size, data);
				break;
			case RenderContextType::Vulkan:
				LUCY_CRITICAL("Vulkan not supported");
				LUCY_ASSERT(false);
				break;
		}
	}

	IndexBuffer::IndexBuffer(uint32_t size, void* data)
		: Buffer(size)
	{
		m_Data = data;
	}

	void IndexBuffer::SetData(void* data)
	{
		m_Data = data;
	}
}
