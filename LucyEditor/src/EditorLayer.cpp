#include <iostream>
#include "EditorLayer.h"
#include "Core/Base.h"

#include "Renderer/Renderer.h"
#include "Scene/Entity.h"
#include "Scene/Components.h"
#include "Renderer/Buffer/FrameBuffer.h"
#include "Renderer/Buffer/OpenGL/OpenGLFrameBuffer.h"

#include "Events/KeyCodes.h"

#include "Renderer/RenderCommand.h"
#include "glad/glad.h"

namespace Lucy {

	void EditorLayer::Begin()
	{
		Scene& scene = Renderer::GetActiveScene();
		auto& meshView = scene.registry.view<MeshComponent>();

		for (auto entity : meshView) {
			Entity e{ &scene, entity };
			MeshComponent& meshComponent = e.GetComponent<MeshComponent>();
		}
	}

	void EditorLayer::End()
	{
		Renderer::Dispatch();

		GLenum state = glGetError();
		if (state != GL_NO_ERROR) Logger::Log(LoggerInfo::LUCY_CRITICAL, state);
	}

	void EditorLayer::Init(GLFWwindow* window)
	{
		m_Window = window;
	}

	void EditorLayer::OnRender()
	{
		auto& mainFrameBuffer = Renderer::GetMainFrameBuffer();
		Renderer::Submit([&]() {
			mainFrameBuffer->Bind();
			RenderCommand::ClearColor(1.0f, 0.5f, 0.5f, 1.0f);
			RenderCommand::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			mainFrameBuffer->Blit();
			mainFrameBuffer->Unbind();
		});
	}

	void EditorLayer::OnEvent(Event& e)
	{
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
				glfwSetWindowShouldClose(m_Window, GL_TRUE);
			}
		});
	}

	void EditorLayer::Destroy()
	{
	}
}