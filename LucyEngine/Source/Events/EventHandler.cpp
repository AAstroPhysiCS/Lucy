#include "lypch.h"
#include "EventHandler.h"

namespace Lucy {

	bool Input::IsMousePressed(MouseCode mouseCode) {
		LUCY_ASSERT(EventHandler::s_RawWindowPtr, "Window Ptr is null! Call Init()");
		return glfwGetMouseButton(EventHandler::s_RawWindowPtr, (int32_t)mouseCode) == GLFW_PRESS;
	}

	bool Input::IsKeyPressed(KeyCode keyCode) {
		LUCY_ASSERT(EventHandler::s_RawWindowPtr, "Window Ptr is null! Call Init()");
		return glfwGetKey(EventHandler::s_RawWindowPtr, (int32_t)keyCode) == GLFW_PRESS;
	}

	bool Input::IsMouseRelease(KeyCode mouseCode) {
		LUCY_ASSERT(EventHandler::s_RawWindowPtr, "Window Ptr is null! Call Init()");
		return glfwGetMouseButton(EventHandler::s_RawWindowPtr, (int32_t)mouseCode) == GLFW_RELEASE;
	}

	bool Input::IsKeyRelease(KeyCode keyCode) {
		LUCY_ASSERT(EventHandler::s_RawWindowPtr, "Window Ptr is null! Call Init()");
		return glfwGetKey(EventHandler::s_RawWindowPtr, (int32_t)keyCode) == GLFW_RELEASE;
	}

	void EventHandler::Init(Application* appPtr, GLFWwindow* windowPtr) {
		s_RawWindowPtr = windowPtr;
		s_RawAppPtr = appPtr;
	}
}