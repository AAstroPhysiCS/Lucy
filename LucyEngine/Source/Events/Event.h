#pragma once

namespace Lucy {

	enum class EventType {
		MouseEvent, KeyEvent, CharCallbackEvent, ScrollEvent, CursorPosEvent,
		WindowResizeEvent, WindowCloseEvent, CustomEvent, Unknown
	};

	class Event {
	public:
		Event() = default;
		virtual ~Event() = default;

		EventType GetType() { return m_Type; }
	protected:
		EventType m_Type = EventType::Unknown;
	};
}
