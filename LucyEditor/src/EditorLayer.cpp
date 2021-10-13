#include "EditorLayer.h"

#include "Scene/Components.h"

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
		Renderer::Dispatch();
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