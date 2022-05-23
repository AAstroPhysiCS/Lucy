#include "EditorModule.h"

#include "Renderer/Renderer.h"
#include "Renderer/RenderPass.h"
#include "Scene/Entity.h"
#include "Scene/Components.h"
#include "Renderer/Buffer/FrameBuffer.h"
#include "Renderer/Buffer/OpenGL/OpenGLFrameBuffer.h"

#include "UI/ViewportPanel.h"
#include "UI/SceneHierarchyPanel.h"

#include "Events/KeyCodes.h"
#include "Events/MouseCode.h"
#include "Events/WindowEvent.h"
#include "Events/InputEvent.h"

#include "glad/glad.h"

namespace Lucy {

	void EditorModule::Init(RefLucy<Window> window) {
		m_Window = window;
		m_ViewportRenderer.Init();
	}

	void EditorModule::Begin(PerformanceMetrics& rendererMetrics) {
		m_Scene.GetEditorCamera().OnEvent(rendererMetrics);
		m_ViewportRenderer.Begin(m_Scene);
	}

	void EditorModule::OnRender() {
		m_ViewportRenderer.Dispatch();
	}

	void EditorModule::End() {
		m_ViewportRenderer.End();
	}

	void EditorModule::OnEvent(Event& e) {
		EventDispatcher& dispatcher = EventDispatcher::GetInstance();
		dispatcher.Dispatch<KeyEvent>(e, EventType::KeyEvent, [&](const KeyEvent& e) {
			if (e == KeyCode::Escape) {
				glfwSetWindowShouldClose(m_Window->Raw(), GL_TRUE);
			}
		});

		dispatcher.Dispatch<WindowResizeEvent>(e, EventType::WindowResizeEvent, [&](const WindowResizeEvent& e) {
			m_ViewportRenderer.OnWindowResize();
		});

		dispatcher.Dispatch<MouseEvent>(e, EventType::MouseEvent, [](const MouseEvent& e) {
			const ViewportPanel& viewportPanel = ViewportPanel::GetInstance();
			if (viewportPanel.IsOverAnyGizmo || !viewportPanel.IsViewportActive) return;

			if (e == MouseCode::Button0) {
				Entity e = Renderer::OnMousePicking();
				SceneHierarchyPanel& sceneHierarchyPanel = SceneHierarchyPanel::GetInstance();
				e.IsValid() ? sceneHierarchyPanel.SetEntityContext(e) : sceneHierarchyPanel.SetEntityContext({});
			}
		});
	}

	void EditorModule::Destroy() {
		m_ViewportRenderer.Destroy();
	}
}