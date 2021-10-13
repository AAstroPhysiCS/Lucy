#pragma once

#include "Renderer/Renderer.h"
#include "EditorLayer.h"
#include "ImGuiLayer.h"

#include "Core/Application.h"
#include "Core/Window.h"

namespace Lucy {

	class EditorApplication : public Application
	{
	public:
		EditorApplication(const ApplicationArgs& args);
		~EditorApplication();
		
		void Run();
	private:
		bool m_Running;
		ScopeLucy<Window> m_Window;
	};

	//Creates an application with default parameters
	Application* CreateEditorApplication(const ApplicationArgs& args) {
		return new EditorApplication(args);
	}
}