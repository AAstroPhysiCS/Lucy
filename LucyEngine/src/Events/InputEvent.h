#pragma once

#include "Event.h"
#include "KeyCodes.h"
#include "MouseCode.h"
#include "GLFW/glfw3.h"

namespace Lucy {

	struct CursorPosEvent : Event {

		CursorPosEvent(GLFWwindow* windowPtr, double xPos, double yPos)
			: m_Window(windowPtr), m_XPos(xPos), m_YPos(yPos) {
			m_Type = EventType::CursorPosEvent;
		}

		std::function<void(double, double)> dispatchFunc;

		inline double GetXPos() const { return m_XPos; }
		inline double GetYPos() const { return m_YPos; }
		inline GLFWwindow* GetWindowHandle() const { return m_Window; }
	private:
		double m_XPos;
		double m_YPos;

		GLFWwindow* m_Window;
	};

	struct ScrollEvent : Event {

		ScrollEvent(GLFWwindow* windowPtr, double xOffset, double yOffset)
			: m_Window(windowPtr), m_XOffset(xOffset), m_YOffset(yOffset) {
			m_Type = EventType::ScrollEvent;
		}

		std::function<void(double, double)> dispatchFunc;

		inline double GetXOffset() const { return m_XOffset; }
		inline double GetYOffset() const { return m_YOffset; }
		inline GLFWwindow* GetWindowHandle() const { return m_Window; }
	private:
		double m_XOffset;
		double m_YOffset;
		GLFWwindow* m_Window;
	};

	struct CharCallbackEvent : Event {

		CharCallbackEvent(GLFWwindow* windowPtr, uint32_t codePoint)
			: m_Window(windowPtr), m_CodePoint(codePoint) {
			m_Type = EventType::CharCallbackEvent;
		}

		std::function<void(uint32_t)> dispatchFunc;

		inline int32_t GetCodePoint() const { return m_CodePoint; }
		inline GLFWwindow* GetWindowHandle() const { return m_Window; }
	private:
		uint32_t m_CodePoint;
		GLFWwindow* m_Window;
	};

	struct KeyEvent : Event {

		KeyEvent(GLFWwindow* windowPtr, int32_t key, int32_t scanCode, int32_t action, int32_t mods)
			: m_Window(windowPtr), m_Key(key), m_ScanCode(scanCode), m_Action(action), m_Mods(mods) {
			m_Type = EventType::KeyEvent;
		}

		std::function<void(int32_t, int32_t, int32_t, int32_t)> dispatchFunc;

		inline constexpr bool operator==(KeyCode keyCode) {
			return m_Key == (uint16_t)keyCode && m_Action == GLFW_PRESS;
		}

		inline constexpr bool operator==(KeyCode keyCode) const {
			return m_Key == (uint16_t)keyCode && m_Action == GLFW_PRESS;
		}

		inline int32_t GetKey() const { return m_Key; }
		inline int32_t GetScanCode() const { return m_ScanCode; }
		inline int32_t GetAction() const { return m_Action; }
		inline int32_t GetMods() const { return m_Mods; }
		inline GLFWwindow* GetWindowHandle() const { return m_Window; }
	private:
		int32_t m_Key;
		int32_t m_ScanCode;
		int32_t m_Action;
		int32_t m_Mods;
		GLFWwindow* m_Window;
	};

	struct MouseEvent : Event {

		MouseEvent(GLFWwindow* windowPtr, int32_t button, int32_t action, int32_t mods)
			: m_Window(windowPtr), m_Button(button), m_Action(action), m_Mods(mods) {
			m_Type = EventType::MouseEvent;
		}

		inline constexpr bool operator==(MouseCode mouseCode) {
			return m_Button == (uint16_t)mouseCode && m_Action == GLFW_PRESS;
		}

		inline constexpr bool operator==(MouseCode mouseCode) const {
			return m_Button == (uint16_t)mouseCode && m_Action == GLFW_PRESS;
		}

		std::function<void(int32_t, int32_t, int32_t)> dispatchFunc;

		inline int32_t GetButton() const { return m_Button; }
		inline int32_t GetAction() const { return m_Action; }
		inline int32_t GetMods() const { return m_Mods; }
		inline GLFWwindow* GetWindowHandle() const { return m_Window; }
	private:
		int32_t m_Button;
		int32_t m_Action;
		int32_t m_Mods;
		GLFWwindow* m_Window;
	};
}