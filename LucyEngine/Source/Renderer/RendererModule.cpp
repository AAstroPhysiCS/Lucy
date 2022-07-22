#include "lypch.h"
#include "RendererModule.h"

#include "Events/EventDispatcher.h"
#include "Events/WindowEvent.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	RendererModule::RendererModule(RenderArchitecture arch, Ref<Window> window, Ref<Scene> scene)
		: Module(window, scene) {
		window->SetTitle(fmt::format("{0} - Windows x64 {1}", m_Window->GetTitle(),
						 arch == RenderArchitecture::Vulkan ? "Vulkan" : "DirectX12").c_str());
		m_ViewportRenderer.Init(arch, m_Window);
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

	void RendererModule::Wait() {
		m_ViewportRenderer.WaitForDevice();
	}
}
