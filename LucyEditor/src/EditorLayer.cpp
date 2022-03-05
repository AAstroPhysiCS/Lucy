#include "EditorLayer.h"

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

#include "glad/glad.h"

namespace Lucy {

	void EditorLayer::Init(RefLucy<Window> window) {
		m_Window = window;
	}

	void EditorLayer::Begin(PerformanceMetrics& rendererMetrics) {
		const auto& meshView = m_Scene.View<MeshComponent>();

		for (auto entity : meshView) {
			Entity e{ &m_Scene, entity };
			MeshComponent& meshComponent = e.GetComponent<MeshComponent>();
			if (!meshComponent.IsValid()) continue;

			Renderer::SubmitMesh(meshComponent.GetMesh(), e.GetComponent<TransformComponent>().GetMatrix());
		}

		m_Scene.GetEditorCamera().OnEvent(rendererMetrics);
	}

	void EditorLayer::End() {
		if (Renderer::GetCurrentRenderArchitecture() == RenderArchitecture::OpenGL) {
			GLenum state = glGetError();
			if (state != GL_NO_ERROR) Logger::Log(LoggerInfo::LUCY_CRITICAL, state);
		}
	}

	void EditorLayer::OnRender() {
		Renderer::BeginScene(m_Scene);
		Renderer::Dispatch();
		Renderer::EndScene();

		Renderer::ClearDrawCommands();
	}

	void EditorLayer::OnEvent(Event& e) {
		EventDispatcher& dispatcher = EventDispatcher::GetInstance();
		dispatcher.Dispatch<KeyEvent>(e, EventType::KeyEvent, [&](const KeyEvent& e) {
			if (e == KeyCode::Escape) {
				glfwSetWindowShouldClose(m_Window->Raw(), GL_TRUE);
			}
		});

		dispatcher.Dispatch<MouseEvent>(e, EventType::MouseEvent, [&](const MouseEvent& e) {
			const ViewportPanel& viewportPanel = ViewportPanel::GetInstance();
			if (viewportPanel.IsOverAnyGizmo || !viewportPanel.IsViewportActive) return;

			if (e == MouseCode::Button0) {
				Entity e = Renderer::OnMousePicking();
				SceneHierarchyPanel& sceneHierarchyPanel = SceneHierarchyPanel::GetInstance();
				e.IsValid() ? sceneHierarchyPanel.SetEntityContext(e) : sceneHierarchyPanel.SetEntityContext({});
			}
		});
	}

	void EditorLayer::Destroy() {

	}
}