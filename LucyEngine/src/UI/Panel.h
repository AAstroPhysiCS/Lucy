#pragma once

#include "Events/Event.h"

namespace Lucy {

	class Panel {
	public:
		virtual void OnEvent(Event& e) {}
		virtual void Render() = 0;
	};
}