#include "EditorLayer.h"

namespace Lucy {

	void EditorLayer::Begin()
	{
		auto& rendererAPI = Renderer::GetRendererAPI();

		//rendererAPI->ClearColor(0.0f, 0.0f, 0.0f, 1.0f); //delete after a separate framebuffer initialization
		//rendererAPI->Clear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	}

	void EditorLayer::End()
	{
		//later
	}

	void EditorLayer::OnRender()
	{
		//later
	}

	void EditorLayer::OnEvent(Event& e)
	{

		/*
		EventDispatcher* dispatcher = EventDispatcher::GetInstance();
		dispatcher->Dispatch<KeyEvent>(e, []()  {
			Logger::Log(LoggerInfo::LUCY_INFO, "HELLO");
		});

		dispatcher->Dispatch<KeyEvent>(e, []() {
			Logger::Log(LoggerInfo::LUCY_INFO, "fuck");
		});

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
		*/
	}

	void EditorLayer::Destroy()
	{
		EventDispatcher::GetInstance()->Destroy();
	}

}