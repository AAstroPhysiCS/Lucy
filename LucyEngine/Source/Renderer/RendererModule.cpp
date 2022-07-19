#include "lypch.h"
#include "RendererModule.h"

#include "Events/EventDispatcher.h"
#include "Events/WindowEvent.h"

namespace Lucy {

	RendererModule::RendererModule(Ref<Window> window, Ref<Scene> scene)
		: Module(window, scene) {
		m_ViewportRenderer.Init();
	}

	void RendererModule::Begin() {
		m_ViewportRenderer.Begin(*m_Scene.Get());
	}

	void RendererModule::OnRender() {
		m_ViewportRenderer.Dispatch(*m_Scene.Get());
	}

	void RendererModule::End() {
		m_ViewportRenderer.End();
	}
	
	void RendererModule::OnEvent(Event& e) {
		EventDispatcher& dispatcher = EventDispatcher::GetInstance();
		dispatcher.Dispatch<WindowResizeEvent>(e, EventType::WindowResizeEvent, [&](const WindowResizeEvent& e) {
			m_ViewportRenderer.OnWindowResize();
		});
	}

	void RendererModule::Destroy() {
		m_ViewportRenderer.Destroy();
	}
}
