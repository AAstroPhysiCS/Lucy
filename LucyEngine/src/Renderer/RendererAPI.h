#pragma once

#include "../Core/Base.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"

namespace Lucy {

	class RendererAPI
	{

	public:

		virtual void SwapBuffers(GLFWwindow* window) = 0;
		virtual void Clear(uint32_t bitField) = 0;
		virtual void ClearColor(float r, float g, float b, float a) = 0;

		template <class T>
		static RefLucy<T> Create()
		{

			if (!std::is_base_of<RendererAPI, T>::value) {
				LUCY_CRITICAL("Renderer API must be a child class");
				LUCY_ASSERT(false);
			}

			switch (Renderer::GetCurrentContext()) {
				case RendererContext::OPENGL:
					return CreateRef<OpenGLRendererAPI>();
				default:
					LUCY_CRITICAL("API not supported!");
					LUCY_ASSERT(false);
			}
		}
	};

}