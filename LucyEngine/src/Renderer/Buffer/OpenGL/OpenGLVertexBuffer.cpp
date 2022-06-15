#include "lypch.h"
#include "OpenGLVertexBuffer.h"
#include "glad/glad.h"

namespace Lucy {

	OpenGLVertexBuffer::OpenGLVertexBuffer(uint32_t size)
		: VertexBuffer(size) {
		glCreateBuffers(1, &m_Id);
	}

	OpenGLVertexBuffer::OpenGLVertexBuffer() {
		glCreateBuffers(1, &m_Id);
	}

	void OpenGLVertexBuffer::Bind(const VertexBindInfo& info) {
		//info is vulkan only
		glBindBuffer(GL_ARRAY_BUFFER, m_Id);
	}

	void OpenGLVertexBuffer::Unbind() {
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void OpenGLVertexBuffer::LoadToGPU() {
		Bind({});
		glBufferData(GL_ARRAY_BUFFER, m_Data.size() * sizeof(float), m_Data.data(), GL_STATIC_DRAW);
		Unbind();
	}

	void OpenGLVertexBuffer::DestroyHandle() {
		glDeleteBuffers(1, &m_Id);
	}
}