#include "lypch.h"
#include "OpenGLIndexBuffer.h"

#include <iostream>
#include <algorithm>

#include "glad/glad.h"

namespace Lucy {

	OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t size)
		: IndexBuffer(size) {
		glCreateBuffers(1, &m_Id);
	}

	OpenGLIndexBuffer::OpenGLIndexBuffer() {
		glCreateBuffers(1, &m_Id);
	}

	void OpenGLIndexBuffer::Bind(const IndexBindInfo& info) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Id);
	}

	void OpenGLIndexBuffer::Unbind() {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	void OpenGLIndexBuffer::LoadToGPU() {
		Bind({});
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Data.size() * sizeof(uint32_t), m_Data.data(), GL_STATIC_DRAW);
		Unbind();
	}

	void OpenGLIndexBuffer::DestroyHandle() {
		glDeleteBuffers(1, &m_Id);
	}
}