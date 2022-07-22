#pragma once

#include "../Events/Event.h"

#include "Window.h"
#include "Scene/Scene.h"

namespace Lucy {

	class Module {
	public:
		virtual ~Module() = default;
		
		virtual void Begin() = 0;
		virtual void End() = 0;
		virtual void OnRender() = 0;
		virtual void OnEvent(Event& e) = 0;
		virtual void Destroy() = 0;
		virtual void Wait() = 0;
	protected:
		Module(Ref<Window> window, Ref<Scene> scene);
		
		Ref<Window> m_Window = nullptr;
		Ref<Scene> m_Scene = nullptr;
	};
}