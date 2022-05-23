#pragma once

#include "Events/Event.h"
#include "Events/EventDispatcher.h"

namespace Lucy {

	class Panel {
	public:
		virtual void OnEvent(Event& e) {}
		virtual void Render() = 0;
	};
}