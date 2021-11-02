#include "OpenGLIndexBuffer.h"

#include <iostream>
#include <algorithm>

#include "glad/glad.h"

namespace Lucy {
	
	OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t size)
		: IndexBuffer(size)
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

	void OpenGLIndexBuffer::Load()
	{
		Bind();
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Data.size() * sizeof(uint32_t), m_Data.data(), GL_STATIC_DRAW);
		Unbind();
	}

	void OpenGLIndexBuffer::Destroy()
	{
		glDeleteBuffers(1, &m_Id);
	}

	void OpenGLIndexBuffer::AddData(std::vector<uint32_t>& dataToAdd)
	{
		m_Data.insert(m_Data.end(), dataToAdd.begin(), dataToAdd.end());
	}
}