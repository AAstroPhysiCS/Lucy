#pragma once

#include "Mesh.h"

namespace Lucy {

	class OpenGLMesh : public Mesh {
	public:
		OpenGLMesh(const std::string& path);
		OpenGLMesh(const OpenGLMesh& other) = default;
		virtual ~OpenGLMesh();

		void Bind();
		void Unbind();
	private:
		void LoadBuffers();

		uint32_t m_Vao = -1;
	};
}