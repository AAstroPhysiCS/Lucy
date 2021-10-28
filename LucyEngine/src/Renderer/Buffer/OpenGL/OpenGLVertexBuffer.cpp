#include "OpenGLVertexBuffer.h"
#include "glad/glad.h"

namespace Lucy {
	
	OpenGLVertexBuffer::OpenGLVertexBuffer(uint32_t size)
		: VertexBuffer(size)
	{
		glCreateBuffers(1, &m_Id);
	}
	
	void OpenGLVertexBuffer::Bind()
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_Id);
	}

	void OpenGLVertexBuffer::Unbind()
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}
