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

	void OpenGLVertexBuffer::Bind() {
		glBindBuffer(GL_ARRAY_BUFFER, m_Id);
	}

	void OpenGLVertexBuffer::Unbind() {
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void OpenGLVertexBuffer::Load() {
		Bind();
		glBufferData(GL_ARRAY_BUFFER, m_Data.size() * sizeof(float), m_Data.data(), GL_STATIC_DRAW);
		Unbind();
	}

	void OpenGLVertexBuffer::Destroy() {
		glDeleteBuffers(1, &m_Id);
	}

	void OpenGLVertexBuffer::AddData(std::vector<float>& dataToAdd) {
		m_Data.insert(m_Data.end(), dataToAdd.begin(), dataToAdd.end());
	}
}
