#pragma once

#include "Core/Base.h" //for child classes

#include "Events/Event.h"
#include "Events/KeyCodes.h" //for child classes
#include "Events/InputEvent.h" //for child classes
#include "Events/WindowEvent.h" //for child classes
#include "Events/MouseCode.h" //for child classes

namespace Lucy {

	class Panel {
	public:
		Panel() = default;
		virtual ~Panel() = default;

		virtual void OnEvent(Event& e) {}
		virtual void OnDestroy() {}
		virtual void Render() = 0;
	};
}