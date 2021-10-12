#pragma once

#include "../../Core/Base.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"

namespace Lucy {

	class RendererAPI
	{
	public:

		virtual void SwapBuffers(GLFWwindow* window) = 0;
		virtual void Clear(uint32_t bitField) = 0;
		virtual void ClearColor(float r, float g, float b, float a) = 0;

		static RefLucy<RendererAPI> Create();

	protected:
		RendererAPI() = default;
		~RendererAPI() = default;
	};

}