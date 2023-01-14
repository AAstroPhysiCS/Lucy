#include "lypch.h"
#include "InputHandler.h"

namespace Lucy {

	void InputHandler::Init(GLFWwindow* windowRawPtr) {
		m_RawWindowPtr = windowRawPtr;
	}

	bool InputHandler::IsMousePressed(MouseCode mouseCode) {
		LUCY_ASSERT(m_RawWindowPtr, "Window Ptr is null! Call Init()");
		return glfwGetMouseButton(m_RawWindowPtr, (int32_t)mouseCode) == GLFW_PRESS;
	}

	bool InputHandler::IsKeyPressed(KeyCode keyCode) {
		LUCY_ASSERT(m_RawWindowPtr, "Window Ptr is null! Call Init()");
		return glfwGetKey(m_RawWindowPtr, (int32_t)keyCode) == GLFW_PRESS;
	}

	bool InputHandler::IsMouseRelease(KeyCode mouseCode) {
		LUCY_ASSERT(m_RawWindowPtr, "Window Ptr is null! Call Init()");
		return glfwGetMouseButton(m_RawWindowPtr, (int32_t)mouseCode) == GLFW_RELEASE;
	}

	bool InputHandler::IsKeyRelease(KeyCode keyCode) {
		LUCY_ASSERT(m_RawWindowPtr, "Window Ptr is null! Call Init()");
		return glfwGetKey(m_RawWindowPtr, (int32_t)keyCode) == GLFW_RELEASE;
	}
}