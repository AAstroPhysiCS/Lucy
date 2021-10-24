#include "OpenGLIndexBuffer.h"

#include "glad/glad.h"

namespace Lucy {
	
	OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t size, void* data)
		: IndexBuffer(size, data)
	{
		glCreateBuffers(1, &m_Id);
	}

	void OpenGLIndexBuffer::Bind()
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Id);
	}

	void OpenGLIndexBuffer::Unbind()
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
}