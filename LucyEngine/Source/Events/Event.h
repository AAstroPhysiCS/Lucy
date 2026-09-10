#pragma once

namespace Lucy {
	
	enum class EventType {
		Unknown,
		KeyEvent,
		MouseEvent,
		CharCallbackEvent,
		ScrollEvent,
		CursorPosEvent,
		EntityPickedEvent,

		WindowResizeEvent,
		WindowCloseEvent,
		SwapChainResizeEvent,
		ViewportAreaResizeEvent
	};

	struct Event {
		Event(EventType type)
			: m_EventType(type) {
		}
		virtual ~Event() = default;

		Event(const Event&) = delete;
		Event& operator=(const Event&) = delete;
		Event(Event&&) = delete;
		Event& operator=(Event&&) = delete;

		inline EventType GetEventType() const { return m_EventType; }
	private:
		EventType m_EventType = EventType::Unknown;
	};
}