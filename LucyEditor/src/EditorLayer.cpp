#include <iostream>
#include "EditorLayer.h"

#include "Renderer/Renderer.h"
#include "Scene/Scene.h"
#include "Scene/Components.h"
#include "Renderer/Buffer/FrameBuffer.h"
#include "Renderer/Buffer/OpenGL/OpenGLFrameBuffer.h"

namespace Lucy {

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
		auto& mainFrameBuffer = Renderer::GetMainFrameBuffer();
		mainFrameBuffer->Bind();
		
		Renderer::Submit([&]() {
			glClearColor(1.0f, 0.5f, 0.5f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			mainFrameBuffer->Blit();
		});
		
		Renderer::Dispatch();
		mainFrameBuffer->Unbind();
	}

	void EditorLayer::OnRender()
	{
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
	}

	void EditorLayer::Destroy()
	{
	}
}