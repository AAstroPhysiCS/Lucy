#pragma once

#include "RendererAPI.h"

namespace Lucy {

	class OpenGLRendererAPI : public RendererAPI {
	public:
		OpenGLRendererAPI() = default;
		virtual ~OpenGLRendererAPI() = default;

		void Clear(uint32_t bitField);
		void ClearColor(float r, float g, float b, float a);
		void SwapBuffers(GLFWwindow* window);

		void DrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices);
		void DrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const void* indices, GLint basevertex);
	};
}