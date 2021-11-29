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
		void ReadPixels(uint32_t x, uint32_t y, uint32_t width, uint32_t height, float* pixelValueOutput);
		void ReadBuffer(uint32_t mode);
		void ReadBuffer(RefLucy<FrameBuffer> frameBuffer, uint32_t mode);

		void DrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices);
		void DrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const void* indices, GLint basevertex);
	};
}