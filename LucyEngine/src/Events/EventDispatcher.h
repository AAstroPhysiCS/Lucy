#pragma once

#include "../Core/Base.h"
#include "Event.h"

namespace Lucy {

	struct EventDispatcher {
		static EventDispatcher& GetInstance() {
			static EventDispatcher s_Instance;
			return s_Instance;
		}

		template <typename T>
		static void Dispatch(Event& evt, EventType type, std::function<void(T&)>&& fnc) {
			if (evt.GetType() == type) {
				fnc((T&)evt);
			}
		}
	};
}