#pragma once

namespace Lucy {

	enum class EventType {
		MouseEvent, KeyEvent, CharCallbackEvent, ScrollEvent, CursorPosEvent,
		WindowResizeEvent, WindowCloseEvent
	};

	class Event {
	public:
		EventType GetType() { return m_Type; }
	protected:
		EventType m_Type;
	};
}
