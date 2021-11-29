#include "lypch.h"

#include "Input.h"
#include "Renderer/Renderer.h"

namespace Lucy {

	float Input::MouseX = 0;
	float Input::MouseY = 0;

	bool Input::IsMousePressed(MouseCode mouseCode) {
		return glfwGetMouseButton(Renderer::s_Window->Raw(), (int32_t)mouseCode) == GLFW_PRESS;
	}

	bool Input::IsKeyPressed(KeyCode keyCode) {
		return glfwGetKey(Renderer::s_Window->Raw(), (int32_t)keyCode) == GLFW_PRESS;
	}

	bool Input::IsMouseRelease(KeyCode mouseCode) {
		return glfwGetMouseButton(Renderer::s_Window->Raw(), (int32_t)mouseCode) == GLFW_RELEASE;
	}

	bool Input::IsKeyRelease(KeyCode keyCode) {
		return glfwGetKey(Renderer::s_Window->Raw(), (int32_t)keyCode) == GLFW_RELEASE;
	}
}