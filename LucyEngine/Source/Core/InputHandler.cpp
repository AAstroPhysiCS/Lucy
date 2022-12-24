#include "lypch.h"
#include "InputHandler.h"

namespace Lucy {

#define CHECK_FOR_NULL() if (!m_RawWindowPtr) { \
								LUCY_CRITICAL("Window Ptr is null! Call Init()"); \
								LUCY_ASSERT(false); \
							} \

	void InputHandler::Init(GLFWwindow* windowRawPtr) {
		m_RawWindowPtr = windowRawPtr;
	}

	bool InputHandler::IsMousePressed(MouseCode mouseCode) {
		CHECK_FOR_NULL();
		return glfwGetMouseButton(m_RawWindowPtr, (int32_t)mouseCode) == GLFW_PRESS;
	}

	bool InputHandler::IsKeyPressed(KeyCode keyCode) {
		CHECK_FOR_NULL();
		return glfwGetKey(m_RawWindowPtr, (int32_t)keyCode) == GLFW_PRESS;
	}

	bool InputHandler::IsMouseRelease(KeyCode mouseCode) {
		CHECK_FOR_NULL();
		return glfwGetMouseButton(m_RawWindowPtr, (int32_t)mouseCode) == GLFW_RELEASE;
	}

	bool InputHandler::IsKeyRelease(KeyCode keyCode) {
		CHECK_FOR_NULL();
		return glfwGetKey(m_RawWindowPtr, (int32_t)keyCode) == GLFW_RELEASE;
	}
}