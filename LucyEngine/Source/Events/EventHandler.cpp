#include "lypch.h"
#include "EventHandler.h"

namespace Lucy {

	bool Input::IsMousePressed(MouseCode mouseCode) {
		LUCY_ASSERT(s_RawWindow, "Window Ptr is null! Call Init()");
		return glfwGetMouseButton(s_RawWindow, (int32_t)mouseCode) == GLFW_PRESS;
	}

	bool Input::IsKeyPressed(KeyCode keyCode) {
		LUCY_ASSERT(s_RawWindow, "Window Ptr is null! Call Init()");
		return glfwGetKey(s_RawWindow, (int32_t)keyCode) == GLFW_PRESS;
	}

	bool Input::IsMouseRelease(MouseCode mouseCode) {
		LUCY_ASSERT(s_RawWindow, "Window Ptr is null! Call Init()");
		return glfwGetMouseButton(s_RawWindow, (int32_t)mouseCode) == GLFW_RELEASE;
	}

	bool Input::IsKeyRelease(KeyCode keyCode) {
		LUCY_ASSERT(s_RawWindow, "Window Ptr is null! Call Init()");
		return glfwGetKey(s_RawWindow, (int32_t)keyCode) == GLFW_RELEASE;
	}

	void Input::Init(GLFWwindow* window) {
		LUCY_ASSERT(window, "Window ptr is null!");
		s_RawWindow = window;
	}

	void EventQueue::Push(std::unique_ptr<Event> event) {
		std::scoped_lock lock(m_Mutex);
		m_Events.emplace_back(std::move(event));
	}

	bool EventQueue::Empty() const {
		std::scoped_lock lock(m_Mutex);
		return m_Events.empty();
	}
}