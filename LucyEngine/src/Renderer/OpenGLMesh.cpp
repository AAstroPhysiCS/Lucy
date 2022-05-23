#include "lypch.h"
#include "OpenGLMesh.h"

#include "Context/OpenGLPipeline.h"

#include "glad/glad.h"

namespace Lucy {

	OpenGLMesh::OpenGLMesh(const std::string& path)
		: Mesh(path) {

	}

	OpenGLMesh::~OpenGLMesh() {
		glDeleteVertexArrays(1, &m_Vao);
	}

	void OpenGLMesh::Bind() {
		if (!m_Loaded)
			LoadBuffers();

		glBindVertexArray(m_Vao);
		m_IndexBuffer->Bind({});
	}

	void OpenGLMesh::Unbind() {
		m_IndexBuffer->Unbind();
		glBindVertexArray(0);
	}

	void OpenGLMesh::LoadBuffers() {
		glCreateVertexArrays(1, &m_Vao);
		glBindVertexArray(m_Vao);

		m_VertexBuffer->Load();
		m_IndexBuffer->Load();

		glBindVertexArray(0);

		m_Loaded = true;
	}
}