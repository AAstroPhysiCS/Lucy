#include "EditorModule.h"

#include "Renderer/Renderer.h"
#include "Renderer/RenderPass.h"
#include "Scene/Entity.h"
#include "Scene/Components.h"
#include "Renderer/Buffer/FrameBuffer.h"
#include "Renderer/Buffer/OpenGL/OpenGLFrameBuffer.h"

#include "Events/KeyCodes.h"
#include "Events/MouseCode.h"
#include "Events/WindowEvent.h"
#include "Events/InputEvent.h"

#include "glad/glad.h"

namespace Lucy {

	EditorModule::EditorModule(RefLucy<Window> window)
		: Module(window) {
		m_ViewportRenderer.Init();
		m_ImGuiOverlay.Init(m_Window, m_Scene);
	}

	void EditorModule::Begin(PerformanceMetrics& rendererMetrics) {
		m_Scene.GetEditorCamera().OnEvent(rendererMetrics);
		m_ViewportRenderer.Begin(m_Scene);

		m_PerformanceMetrics = &rendererMetrics;
	}

	void EditorModule::OnRender() {
		m_ImGuiOverlay.Render(m_PerformanceMetrics);
		m_ViewportRenderer.Dispatch(m_Scene);
	}

	void EditorModule::End() {
		m_ViewportRenderer.End();
		m_Window->SwapBuffers();
	}

	void EditorModule::OnEvent(Event& e) {
		m_ImGuiOverlay.OnEvent(e);
		
		EventDispatcher& dispatcher = EventDispatcher::GetInstance();
		dispatcher.Dispatch<KeyEvent>(e, EventType::KeyEvent, [&](const KeyEvent& e) {
			if (e == KeyCode::Escape) {
				glfwSetWindowShouldClose(m_Window->Raw(), GL_TRUE);
			}
		});

		dispatcher.Dispatch<WindowResizeEvent>(e, EventType::WindowResizeEvent, [&](const WindowResizeEvent& e) {
			m_ViewportRenderer.OnWindowResize();
		});
	}

	void EditorModule::Destroy() {
		m_ImGuiOverlay.Destroy();
		m_ViewportRenderer.Destroy();
	}
}