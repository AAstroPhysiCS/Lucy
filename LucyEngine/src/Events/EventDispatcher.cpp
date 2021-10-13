#include "EventDispatcher.h"

namespace Lucy {

	std::vector<Event*> EventDispatcher::s_Events;

	EventDispatcher& EventDispatcher::GetInstance()
	{
		static EventDispatcher s_Instance;
		return s_Instance;
	}

	std::vector<Event*>& EventDispatcher::GetEventPool() {
		return s_Events;
	}
}