#pragma once

#include "Event.h"

namespace Lucy {

	struct WindowResizeEvent : Event {

		WindowResizeEvent(GLFWwindow* windowPtr, int32_t width, int32_t height)
			: m_Window(windowPtr), m_Width(width), m_Height(height) {
			m_Type = EventType::WindowResizeEvent;
		}

		inline int32_t GetWidth() const { return m_Width; }
		inline int32_t GetHeight() const { return m_Height; }
		inline GLFWwindow* GetWindowHandle() const { return m_Window; }

		std::function<void(int32_t, int32_t)> dispatchFunc;
	private:
		int32_t m_Width;
		int32_t m_Height;
		GLFWwindow* m_Window;
	};

	struct WindowCloseEvent : Event {

		WindowCloseEvent(GLFWwindow* window)
			: m_Window(window) {
			m_Type = EventType::WindowCloseEvent;
		}
		
		inline GLFWwindow* GetWindowHandle() const { return m_Window; }

		std::function<void()> dispatchFunc;
	private:
		GLFWwindow* m_Window;
	};
}