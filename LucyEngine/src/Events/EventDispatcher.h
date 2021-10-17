#pragma once

#include <vector>
#include <any>
#include <functional>
#include <algorithm>

#include "Event.h"

namespace Lucy {

	class EventDispatcher
	{
	public:
		template <class E>
		constexpr void PushEvent(const E evtFnc) {

			if (!std::is_base_of<Event, E>::value) {
				LUCY_CRITICAL("Event does not inherit from base class");
				LUCY_ASSERT(false);
			}

			s_Events.push_back((Event*) &evtFnc);
		}

		static EventDispatcher& GetInstance();

		template <class E>
		constexpr static void Dispatch(Event& evt, std::function<void()> fnc) {
			
			//type safety
			if (!std::is_base_of<Event, E>::value) {
				LUCY_CRITICAL("Event does not inherit from base class");
				LUCY_ASSERT(false);
			}

			auto result = std::find_if(s_Events.begin(), s_Events.end(), [&](Event* obj) {
				return obj->GetType() != ((E&)evt).GetType();
			});

			if (result != s_Events.end()) { //the event is in the pool (events get deleted afterwards)
				fnc();
			}
		}

		inline static std::vector<Event*>& GetEventPool() { return s_Events; }
	private:
		static std::vector<Event*> s_Events;
	};
}