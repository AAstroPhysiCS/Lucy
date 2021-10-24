#pragma once

#include <cstdint>
#include <functional>

#include "Event.h"
#include "KeyCodes.h"

namespace Lucy {

	struct CursorPosEvent : Event {

		CursorPosEvent(double xPos, double yPos)
			: m_XPos(xPos), m_YPos(yPos) {
			m_Type = EventType::CursorPosEvent;
		}

		std::function<void(double, double)> dispatchFunc;

		inline double GetXPos() const { return m_XPos; }
		inline double GetYPos() const { return m_YPos; }

	private:
		double m_XPos;
		double m_YPos;
	};

	struct ScrollEvent : Event {

		ScrollEvent(double xOffset, double yOffset)
			: m_XOffset(xOffset), m_YOffset(yOffset) {
			m_Type = EventType::ScrollEvent;
		}

		inline double GetXOffset() const { return m_XOffset; }
		inline double GetYOffset() const { return m_YOffset; }

		std::function<void(double, double)> dispatchFunc;

	private:
		double m_XOffset;
		double m_YOffset;
	};

	struct CharCallbackEvent : Event {

		CharCallbackEvent(uint32_t codePoint)
			: m_CodePoint(codePoint) {
			m_Type = EventType::CharCallbackEvent;
		}

		inline int32_t GetCodePoint() const { return m_CodePoint; }

		std::function<void(uint32_t)> dispatchFunc;
	
	private:
		uint32_t m_CodePoint;
	};

	struct KeyEvent : Event {

		KeyEvent(int32_t key, int32_t scanCode, int32_t action, int32_t mods)
			: m_Key(key), m_ScanCode(scanCode), m_Action(action), m_Mods(mods) {
			m_Type = EventType::KeyEvent;
		}

		std::function<void(int32_t, int32_t, int32_t, int32_t)> dispatchFunc;

		inline constexpr bool operator==(KeyCode keyCode) {
			return m_Key == (uint16_t) keyCode;
		}

		inline int32_t GetKey() const { return m_Key; }
		inline int32_t GetScanCode() const { return m_ScanCode; }
		inline int32_t GetAction() const { return m_Action; }
		inline int32_t GetMods() const { return m_Mods; }

	private:
		int32_t m_Key;
		int32_t m_ScanCode;
		int32_t m_Action;
		int32_t m_Mods;
	};

	struct MouseEvent : Event {
		
		MouseEvent(int32_t button, int32_t action, int32_t mods)
			: m_Button(button), m_Action(action), m_Mods(mods) {
			m_Type = EventType::MouseEvent;
		}

		inline int32_t GetButton() const { return m_Button; }
		inline int32_t GetAction() const { return m_Action; }
		inline int32_t GetMods() const { return m_Mods; }

		std::function<void(int32_t, int32_t, int32_t)> dispatchFunc;
	private:
		int32_t m_Button;
		int32_t m_Action;
		int32_t m_Mods;
	};

}