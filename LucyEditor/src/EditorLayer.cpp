#include <iostream>
#include "EditorLayer.h"

#include "Renderer/Renderer.h"
#include "Renderer/RenderPass.h"
#include "Scene/Entity.h"
#include "Scene/Components.h"
#include "Renderer/Buffer/FrameBuffer.h"
#include "Renderer/Buffer/OpenGL/OpenGLFrameBuffer.h"

#include "Events/KeyCodes.h"

#include "Renderer/RenderCommand.h"
#include "glad/glad.h"

namespace Lucy {

	void EditorLayer::Init(RefLucy<Window> window) {
		m_Window = window;
	}

	void EditorLayer::Begin() {
		auto& meshView = m_Scene.View<MeshComponent>();

		for (auto entity : meshView) {
			Entity e{ &m_Scene, entity };
			MeshComponent& meshComponent = e.GetComponent<MeshComponent>();
			if (!meshComponent.IsValid()) continue;

			Renderer::SubmitMesh(meshComponent.GetMesh(), e.GetComponent<TransformComponent>().GetMatrix());
		}
	}

	void EditorLayer::End() {
		GLenum state = glGetError();
		if (state != GL_NO_ERROR) Logger::Log(LoggerInfo::LUCY_CRITICAL, state);
	}

	void EditorLayer::OnRender() {
		Renderer::BeginScene(m_Scene);
		Renderer::Dispatch();
		Renderer::EndScene();

		RenderCommand::SwapBuffers(m_Window->Raw());
		Renderer::ClearDrawCommands();
	}

	void EditorLayer::OnEvent(Event& e) {
		switch (e.GetType()) {
			case EventType::KeyEvent:
				break;
			case EventType::MouseEvent:
				break;
			case EventType::WindowResizeEvent:
				break;
			case EventType::ScrollEvent:
				break;
			case EventType::CursorPosEvent:
				break;
			case EventType::CharCallbackEvent:
				break;
		}

		EventDispatcher& dispatcher = EventDispatcher::GetInstance();
		dispatcher.Dispatch<KeyEvent>(e, EventType::KeyEvent, [&](KeyEvent& e) {
			if (e == KeyCode::Escape) {
				glfwSetWindowShouldClose(m_Window->Raw(), GL_TRUE);
			}
		});
	}

	void EditorLayer::Destroy() {
		Renderer::Destroy();
	}
}