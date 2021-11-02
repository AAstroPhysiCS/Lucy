#include "VertexBuffer.h"

#include "OpenGL/OpenGLVertexBuffer.h"
#include "../Renderer.h"

namespace Lucy {

	RefLucy<VertexBuffer> VertexBuffer::Create(uint32_t size)
	{
		switch (Renderer::GetCurrentRenderAPI()) {
		case RenderAPI::OpenGL:
			return CreateRef<OpenGLVertexBuffer>(size);
			break;
		case RenderAPI::Vulkan:
			LUCY_CRITICAL("Vulkan not supported");
			LUCY_ASSERT(false);
			break;
		}
	}

	RefLucy<VertexBuffer> VertexBuffer::Create()
	{
		switch (Renderer::GetCurrentRenderAPI()) {
		case RenderAPI::OpenGL:
			return CreateRef<OpenGLVertexBuffer>();
			break;
		case RenderAPI::Vulkan:
			LUCY_CRITICAL("Vulkan not supported");
			LUCY_ASSERT(false);
			break;
		}
	}

	VertexBuffer::VertexBuffer(uint32_t size)
		: Buffer<float>(size)
	{
		m_Data.resize(size);
		m_DataHead = m_Data.data();
	}
}

