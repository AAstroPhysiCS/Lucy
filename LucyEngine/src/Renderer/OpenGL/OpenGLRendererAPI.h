#pragma once

#include "../RendererAPI.h"

namespace Lucy {

	class OpenGLRendererAPI : public RendererAPI
	{
	public:

		void Clear(uint32_t bitField);
		void ClearColor(float r, float g, float b, float a);
		void SwapBuffers(GLFWwindow* window);

	};

}