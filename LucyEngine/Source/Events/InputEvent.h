#pragma once

#include "KeyCodes.h"
#include "MouseCode.h"
#include "GLFW/glfw3.h"

#include "Event.h"

namespace Lucy {

	class Entity;
	class Scene;

	struct CursorPosEvent : public Event {
		inline static constexpr const EventType EventType = EventType::CursorPosEvent;

		CursorPosEvent(GLFWwindow* windowPtr, double xPos, double yPos)
			: Event(EventType), m_XPos(xPos), m_YPos(yPos), m_Window(windowPtr) {
		}
		virtual ~CursorPosEvent() = default;

		inline double GetXPos() const { return m_XPos; }
		inline double GetYPos() const { return m_YPos; }
		inline GLFWwindow* GetWindowHandle() const { return m_Window; }
	private:
		double m_XPos;
		double m_YPos;

		GLFWwindow* m_Window;
	};

	struct ScrollEvent : public Event {
		inline static constexpr const EventType EventType = EventType::ScrollEvent;

		ScrollEvent(GLFWwindow* windowPtr, double xOffset, double yOffset)
			: Event(EventType), m_XOffset(xOffset), m_YOffset(yOffset), m_Window(windowPtr) {
		}
		virtual ~ScrollEvent() = default;

		inline double GetXOffset() const { return m_XOffset; }
		inline double GetYOffset() const { return m_YOffset; }
		inline GLFWwindow* GetWindowHandle() const { return m_Window; }
	private:
		double m_XOffset;
		double m_YOffset;

		GLFWwindow* m_Window;
	};

	struct CharCallbackEvent : public Event {
		inline static constexpr const EventType EventType = EventType::CharCallbackEvent;

		CharCallbackEvent(GLFWwindow* windowPtr, uint32_t codePoint)
			: Event(EventType), m_CodePoint(codePoint), m_Window(windowPtr) {
		}
		virtual ~CharCallbackEvent() = default;

		inline int32_t GetCodePoint() const { return m_CodePoint; }
		inline GLFWwindow* GetWindowHandle() const { return m_Window; }
	private:
		uint32_t m_CodePoint;

		GLFWwindow* m_Window;
	};

	struct KeyEvent : public Event {
		inline static constexpr const EventType EventType = EventType::KeyEvent;

		KeyEvent(GLFWwindow* windowPtr, int32_t key, int32_t scanCode, int32_t action, int32_t mods)
			: Event(EventType), m_Key(key), m_ScanCode(scanCode), m_Action(action), m_Mods(mods), m_Window(windowPtr) {
		}
		virtual ~KeyEvent() = default;

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

	struct MouseEvent : public Event {
		inline static constexpr const EventType EventType = EventType::MouseEvent;

		MouseEvent(GLFWwindow* windowPtr, int32_t button, int32_t action, int32_t mods)
			: Event(EventType), m_Button(button), m_Action(action), m_Mods(mods), m_Window(windowPtr) {
		}
		virtual ~MouseEvent() = default;

		inline constexpr bool operator==(MouseCode mouseCode) {
			return m_Button == (uint16_t)mouseCode && m_Action == GLFW_PRESS;
		}

		inline constexpr bool operator==(MouseCode mouseCode) const {
			return m_Button == (uint16_t)mouseCode && m_Action == GLFW_PRESS;
		}

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

	struct EntityPickedEvent : public Event {
		inline static constexpr const EventType EventType = EventType::EntityPickedEvent;

		EntityPickedEvent(Entity& entity, Scene* scene, float viewportMouseX, float viewportMouseY) 
			: Event(EventType), m_Scene(scene), m_Entity(entity), m_ViewportMouseX(viewportMouseX), m_ViewportMouseY(viewportMouseY) {
		}
		virtual ~EntityPickedEvent() = default;

		inline Scene* GetScene() const { return m_Scene; }
		inline Entity& GetEntity() const { return m_Entity; }

		inline float GetViewportMouseX() const { return m_ViewportMouseX; }
		inline float GetViewportMouseY() const { return m_ViewportMouseY; }
	private:
		Scene* m_Scene = nullptr;
		Entity& m_Entity;

		float m_ViewportMouseX = 0, m_ViewportMouseY = 0;
	};
}