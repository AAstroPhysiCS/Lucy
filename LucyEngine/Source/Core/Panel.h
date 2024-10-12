#pragma once

#include "Core/Base.h" //for child classes

#include "Events/KeyCodes.h" //for child classes
#include "Events/MouseCode.h" //for child classes
#include "Events/Event.h"

namespace Lucy {

	class Panel {
	public:
		Panel() = default;
		virtual ~Panel() = default;

		virtual void OnEvent(Event& e) { /* Could be overriden by the corresponding child class */ }
		virtual void OnDestroy() { /* Could be overriden by the corresponding child class */ }
		virtual void Render() = 0;

		inline void ToggleShow() { m_Show = !m_Show; }
		inline bool GetShow() const { return m_Show; }
	private:
		bool m_Show = true;
	};
}