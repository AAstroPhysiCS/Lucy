#pragma once

#include "../Events/Event.h"

#include "Window.h"

namespace Lucy {

	class Module {
	public:
		virtual ~Module() = default;
		
		virtual void Begin() = 0;
		virtual void End() = 0;
		virtual void OnRender() = 0;
		virtual void OnEvent(Event& e) = 0;
		virtual void Destroy() = 0;
	protected:
		Module(Ref<Window> window);
		
		Ref<Window> m_Window = nullptr;
	};
}