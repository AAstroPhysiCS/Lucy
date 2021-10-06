#include "OpenGLRendererAPI.h"

namespace Lucy {

	void OpenGLRendererAPI::SwapBuffers(GLFWwindow* window)
	{
		glfwSwapBuffers(window);
	}

	void OpenGLRendererAPI::ClearColor(float r, float g, float b, float a)
	{
		glClearColor(r, g, b, a);
	}

	void OpenGLRendererAPI::Clear(uint32_t bitField)
	{
		glClear(bitField);
	}

}
