#pragma once

#include "../../Core/Base.h"

#include "glad/glad.h"
#include "GLFW/glfw3.h"

namespace Lucy {

	class FrameBuffer;

	class RendererAPI {
	public:
		virtual void SwapBuffers(GLFWwindow* window) = 0;
		virtual void Clear(uint32_t bitField) = 0;
		virtual void ClearColor(float r, float g, float b, float a) = 0;
		virtual void ReadPixels(uint32_t x, uint32_t y, uint32_t width, uint32_t height, float* pixelValueOutput) = 0;
		virtual void ReadBuffer(RefLucy<FrameBuffer> frameBuffer, uint32_t mode) = 0;
		virtual void ReadBuffer(uint32_t mode) = 0;

		virtual void DrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices) = 0;
		virtual void DrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const void* indices, GLint basevertex) = 0;

		static RefLucy<RendererAPI> Create();
	protected:
		RendererAPI() = default;
		~RendererAPI() = default;
	};
}