#include <iostream>
#include "EditorLayer.h"
#include "Core/Base.h"

#include "Renderer/Renderer.h"
#include "Scene/Scene.h"
#include "Scene/Components.h"
#include "Renderer/Buffer/FrameBuffer.h"
#include "Renderer/Buffer/OpenGL/OpenGLFrameBuffer.h"

#include "Events/KeyCodes.h"

#include "Renderer/RenderCommand.h"
#include "glad/glad.h"

namespace Lucy {

	void EditorLayer::Init(GLFWwindow* window)
	{
		m_Window = window;
	}

	void EditorLayer::Begin()
	{
		Scene& scene = Renderer::GetActiveScene();
		auto& meshView = scene.registry.view<MeshComponent>();

		for (auto entity : meshView) {
			MeshComponent& meshComponent = scene.registry.get<MeshComponent>(entity);
		}
	}

	void EditorLayer::End()
	{
		Renderer::Dispatch();

		GLenum state = glGetError();
		if (state != GL_NO_ERROR) Logger::Log(LoggerInfo::LUCY_CRITICAL, state);
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
				Logger::Log(LoggerInfo::LUCY_INFO, "KeyEvent triggered!");
				break;
			case EventType::MouseEvent:
				Logger::Log(LoggerInfo::LUCY_INFO, "MouseEvent triggered!");
				break;
			case EventType::WindowResizeEvent:
				Logger::Log(LoggerInfo::LUCY_INFO, "WindowResizeEvent triggered!");
				break;
			case EventType::ScrollEvent:
				Logger::Log(LoggerInfo::LUCY_INFO, "ScrollEvent triggered!");
				break;
			case EventType::CursorPosEvent:
				Logger::Log(LoggerInfo::LUCY_INFO, "CursorPosEvent triggered!");
				break;
			case EventType::CharCallbackEvent:
				Logger::Log(LoggerInfo::LUCY_INFO, "CharCallbackEvent triggered!");
				break;
		}

		EventDispatcher& dispatcher = EventDispatcher::GetInstance();
		dispatcher.Dispatch<KeyEvent>(e, EventType::KeyEvent, [=](KeyEvent& e) {
			if (e == KeyCode::Escape) {
				glfwSetWindowShouldClose(m_Window, GL_TRUE);
			}
		});
	}

	void EditorLayer::Destroy()
	{
	}
}