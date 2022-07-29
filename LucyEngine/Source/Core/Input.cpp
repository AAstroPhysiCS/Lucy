#include "lypch.h"
#include "Input.h"

namespace Lucy {

#define CHECK_FOR_NULL() if (!m_RawWindowPtr) { \
								LUCY_CRITICAL("Window Ptr is null! Call Init()"); \
								LUCY_ASSERT(false); \
							} \

	float Input::MouseX = 0;
	float Input::MouseY = 0;

	GLFWwindow* Input::m_RawWindowPtr = nullptr;

	void Input::Init(GLFWwindow* windowRawPtr) {
		m_RawWindowPtr = windowRawPtr;
	}

	bool Input::IsMousePressed(MouseCode mouseCode) {
		CHECK_FOR_NULL();
		return glfwGetMouseButton(m_RawWindowPtr, (int32_t)mouseCode) == GLFW_PRESS;
	}

	bool Input::IsKeyPressed(KeyCode keyCode) {
		CHECK_FOR_NULL();
		return glfwGetKey(m_RawWindowPtr, (int32_t)keyCode) == GLFW_PRESS;
	}

	bool Input::IsMouseRelease(KeyCode mouseCode) {
		CHECK_FOR_NULL();
		return glfwGetMouseButton(m_RawWindowPtr, (int32_t)mouseCode) == GLFW_RELEASE;
	}

	bool Input::IsKeyRelease(KeyCode keyCode) {
		CHECK_FOR_NULL();
		return glfwGetKey(m_RawWindowPtr, (int32_t)keyCode) == GLFW_RELEASE;
	}
}