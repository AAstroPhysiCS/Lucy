#include "EventDispatcher.h"

namespace Lucy {
	
	EventDispatcher* EventDispatcher::s_Instance = nullptr;
	std::vector<Event*> EventDispatcher::s_Events;

	void EventDispatcher::Destroy() {
		s_Events.clear();
		delete s_Instance;
	}

	std::vector<Event*>& EventDispatcher::GetEventPool() {
		return s_Events;
	}
}