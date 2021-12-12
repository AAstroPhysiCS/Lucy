#include "lypch.h"
#include "VertexBuffer.h"

#include "OpenGL/OpenGLVertexBuffer.h"
#include "../Renderer.h"

namespace Lucy {

	RefLucy<VertexBuffer> VertexBuffer::Create(uint32_t size) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return CreateRef<OpenGLVertexBuffer>(size);
				break;
			case RenderArchitecture::Vulkan:
				LUCY_CRITICAL("Vulkan not supported");
				LUCY_ASSERT(false);
				break;
		}
	}

	RefLucy<VertexBuffer> VertexBuffer::Create() {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return CreateRef<OpenGLVertexBuffer>();
				break;
			case RenderArchitecture::Vulkan:
				LUCY_CRITICAL("Vulkan not supported");
				LUCY_ASSERT(false);
				break;
		}
	}

	VertexBuffer::VertexBuffer() {
		m_DataHead = m_Data.data();
	}

	VertexBuffer::VertexBuffer(uint32_t size) {
		m_Data.resize(size);
		m_DataHead = m_Data.data();
	}
}