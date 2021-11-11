#include "IndexBuffer.h"

#include "OpenGL/OpenGLIndexBuffer.h"
#include "../Renderer.h"

namespace Lucy {

	RefLucy<IndexBuffer> IndexBuffer::Create(uint32_t size) {
		switch (Renderer::GetCurrentRenderAPI()) {
			case RenderAPI::OpenGL:
				return CreateRef<OpenGLIndexBuffer>(size);
				break;
			case RenderAPI::Vulkan:
				LUCY_CRITICAL("Vulkan not supported");
				LUCY_ASSERT(false);
				break;
		}
	}

	RefLucy<IndexBuffer> IndexBuffer::Create() {
		switch (Renderer::GetCurrentRenderAPI()) {
			case RenderAPI::OpenGL:
				return CreateRef<OpenGLIndexBuffer>();
				break;
			case RenderAPI::Vulkan:
				LUCY_CRITICAL("Vulkan not supported");
				LUCY_ASSERT(false);
				break;
		}
	}

	IndexBuffer::IndexBuffer() {
		m_DataHead = m_Data.data();
	}

	IndexBuffer::IndexBuffer(uint32_t size) {
		m_Data.resize(size);
		m_DataHead = m_Data.data();
	}
}
