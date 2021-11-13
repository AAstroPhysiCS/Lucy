#include "lypch.h"

#include "OpenGLRendererAPI.h"

namespace Lucy {

	void OpenGLRendererAPI::SwapBuffers(GLFWwindow* window) {
		glfwSwapBuffers(window);
	}

	void OpenGLRendererAPI::DrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices) {
		glDrawElements(mode, count, type, indices);
	}

	void OpenGLRendererAPI::DrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const void* indices, GLint basevertex) {
		glDrawElementsBaseVertex(mode, count, type, indices, basevertex);
	}

	void OpenGLRendererAPI::ClearColor(float r, float g, float b, float a) {
		glClearColor(r, g, b, a);
	}

	void OpenGLRendererAPI::Clear(uint32_t bitField) {
		glClear(bitField);
	}
}