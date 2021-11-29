#include "lypch.h"

#include "OpenGLRendererAPI.h"

#include "Renderer/Buffer/FrameBuffer.h"

namespace Lucy {

	void OpenGLRendererAPI::SwapBuffers(GLFWwindow* window) {
		glfwSwapBuffers(window);
	}

	void OpenGLRendererAPI::ReadBuffer(RefLucy<FrameBuffer> frameBuffer, uint32_t mode) {
		glNamedFramebufferReadBuffer(frameBuffer->GetID(), mode);
	}

	void OpenGLRendererAPI::ReadBuffer(uint32_t mode) {
		glReadBuffer(mode);
	}

	void OpenGLRendererAPI::ReadPixels(uint32_t x, uint32_t y, uint32_t width, uint32_t height, float* pixelValueOutput) {
		glReadPixels(x, y, width, height, GL_RGB, GL_FLOAT, pixelValueOutput);
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