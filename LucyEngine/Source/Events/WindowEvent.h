#pragma once

#include "Event.h"

namespace Lucy {

	struct WindowResizeEvent : public Event {
		inline static constexpr const auto EventType = EventType::WindowResizeEvent;
		
		WindowResizeEvent(GLFWwindow* windowPtr, int32_t width, int32_t height)
			: Event(EventType), m_Width(width), m_Height(height), m_Window(windowPtr) {
		}
		virtual ~WindowResizeEvent() = default;

		inline int32_t GetWidth() const { return m_Width; }
		inline int32_t GetHeight() const { return m_Height; }
		inline GLFWwindow* GetWindowHandle() const { return m_Window; }
	private:
		int32_t m_Width;
		int32_t m_Height;

		GLFWwindow* m_Window;
	};

	struct WindowCloseEvent : public Event {
		inline static constexpr const auto EventType = EventType::WindowCloseEvent;

		WindowCloseEvent(GLFWwindow* window)
			: Event(EventType), m_Window(window) {
		}
		virtual ~WindowCloseEvent() = default;

		inline GLFWwindow* GetWindowHandle() const { return m_Window; }
	private:
		GLFWwindow* m_Window;
	};

	struct SwapChainResizeEvent : public Event {
		inline static constexpr const auto EventType = EventType::SwapChainResizeEvent;

		SwapChainResizeEvent() 
			: Event(EventType) {
		}
		virtual ~SwapChainResizeEvent() = default;
	};

	struct ViewportAreaResizeEvent : public Event {
		inline static constexpr const auto EventType = EventType::ViewportAreaResizeEvent;

		ViewportAreaResizeEvent(uint32_t width, uint32_t height)
			: Event(EventType), m_Width(width), m_Height(height) {
		}
		virtual ~ViewportAreaResizeEvent() = default;

		inline uint32_t GetWidth() const { return m_Width; }
		inline uint32_t GetHeight() const { return m_Height; }
	private:
		uint32_t m_Width;
		uint32_t m_Height;
	};
}