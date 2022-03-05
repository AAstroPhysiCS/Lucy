#pragma once

#include "Core/Base.h"
#include "Core/Application.h"
#include "Core/Window.h"
#include "Core/Metrics.h"

namespace Lucy {

	class EditorApplication : public Application
	{
	public:
		explicit EditorApplication(const ApplicationArgs& args);
		virtual ~EditorApplication();
		
		void Run();
		void OnEvent(Event* e);
	private:
		bool m_Running;
		RefLucy<Window> m_Window;
	};

	//Creates an application with default parameters
	inline Application* CreateEditorApplication(const ApplicationArgs& args) {
		return new EditorApplication(args);
	}
}