#include "IndexBuffer.h"

#include "OpenGL/OpenGLIndexBuffer.h"

namespace Lucy {
	
	RefLucy<IndexBuffer> IndexBuffer::Create(uint32_t size)
	{
		switch (Renderer::GetCurrentRenderContextType()) {
			case RenderContextType::OpenGL:
				return CreateRef<OpenGLIndexBuffer>(size);
				break;
			case RenderContextType::Vulkan:
				LUCY_CRITICAL("Vulkan not supported");
				LUCY_ASSERT(false);
				break;
		}
	}

	RefLucy<IndexBuffer> IndexBuffer::Create()
	{
		switch (Renderer::GetCurrentRenderContextType()) {
		case RenderContextType::OpenGL:
			return CreateRef<OpenGLIndexBuffer>();
			break;
		case RenderContextType::Vulkan:
			LUCY_CRITICAL("Vulkan not supported");
			LUCY_ASSERT(false);
			break;
		}
	}

	IndexBuffer::IndexBuffer(uint32_t size)
		: Buffer<uint32_t>(size)
	{
		m_Data = new uint32_t[size];
	}

	void IndexBuffer::SetData(uint32_t* data, size_t size, uint32_t offset)
	{
		delete[] m_Data;
		m_Size = size;
		m_Data = new uint32_t[m_Size];
		memcpy(m_Data + offset, data, m_Size * sizeof(uint32_t));
	}
}
