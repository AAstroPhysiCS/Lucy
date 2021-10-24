#pragma once

#include <stdint.h>
#include "Event.h"

#include <functional>

namespace Lucy {

	struct WindowResizeEvent : Event {

		WindowResizeEvent(int32_t width, int32_t height)
			: m_Width(width), m_Height(height) {
			m_Type = EventType::WindowResizeEvent;
		}

		inline int32_t GetWidth() const { return m_Width; }
		inline int32_t GetHeight() const { return m_Height; }

		std::function<void(int32_t, int32_t)> dispatchFunc;

	private:
		int32_t m_Width;
		int32_t m_Height;
	};

	struct WindowCloseEvent : Event {

		WindowCloseEvent(GLFWwindow* window)
			: m_Window(window) {
			m_Type = EventType::WindowCloseEvent;
		}

		std::function<void()> dispatchFunc;

	private:
		GLFWwindow* m_Window;
	};
}