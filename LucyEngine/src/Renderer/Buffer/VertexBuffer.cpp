#include "VertexBuffer.h"

#include "OpenGL/OpenGLVertexBuffer.h"

namespace Lucy {

	RefLucy<VertexBuffer> VertexBuffer::Create(uint32_t size)
	{
		switch (Renderer::GetCurrentRenderContextType()) {
		case RenderContextType::OpenGL:
			return CreateRef<OpenGLVertexBuffer>(size);
			break;
		case RenderContextType::Vulkan:
			LUCY_CRITICAL("Vulkan not supported");
			LUCY_ASSERT(false);
			break;
		}
	}

	RefLucy<VertexBuffer> VertexBuffer::Create()
	{
		switch (Renderer::GetCurrentRenderContextType()) {
		case RenderContextType::OpenGL:
			return CreateRef<OpenGLVertexBuffer>();
			break;
		case RenderContextType::Vulkan:
			LUCY_CRITICAL("Vulkan not supported");
			LUCY_ASSERT(false);
			break;
		}
	}

	VertexBuffer::VertexBuffer(uint32_t size)
		: Buffer<float>(size)
	{
		m_Data = new float[size];
	}

	void VertexBuffer::SetData(float* data, size_t size, uint32_t offset)
	{
		delete[] m_Data;
		m_Data = new float[size];
		memcpy(m_Data + offset, data, size * sizeof(float));
	}
}

