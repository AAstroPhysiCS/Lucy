#pragma once

#include <stdint.h>
#include <functional>

#include "Event.h"

namespace Lucy {

	struct CursorPosEvent : Event {

		CursorPosEvent(double xPos, double yPos)
			: m_XPos(xPos), m_YPos(yPos) {
			m_Type = EventType::CursorPosEvent;
		}

		std::function<void(double, double)> dispatchFunc;

	private:
		double m_XPos;
		double m_YPos;
	};

	struct ScrollEvent : Event {

		ScrollEvent(double xOffset, double yOffset)
			: m_XOffset(xOffset), m_YOffset(yOffset) {
			m_Type = EventType::ScrollEvent;
		}

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

		std::function<void(int32_t, int32_t, int32_t)> dispatchFunc;

	private:
		int32_t m_Button;
		int32_t m_Action;
		int32_t m_Mods;
	};

}