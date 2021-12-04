#include "lypch.h"
#include "OpenGLUniformBuffer.h"

#include "Renderer/Renderer.h"

namespace Lucy {
	OpenGLUniformBuffer::OpenGLUniformBuffer(uint32_t size, uint32_t binding) {
		Renderer::Submit([=]() {
			glCreateBuffers(1, &m_Id);
			glNamedBufferData(m_Id, size, nullptr, GL_DYNAMIC_DRAW);
			glBindBufferBase(GL_UNIFORM_BUFFER, binding, m_Id);
		});
	}
	
	void OpenGLUniformBuffer::Bind() {
		glBindBuffer(GL_UNIFORM_BUFFER, m_Id);
	}

	void OpenGLUniformBuffer::Unbind() {
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	void OpenGLUniformBuffer::SetData(void* data, uint32_t size, uint32_t offset) {
		glNamedBufferSubData(m_Id, offset, size, data);
	}
}