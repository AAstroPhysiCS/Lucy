#pragma once

#include "Application.h"
#include "Renderer.h"

namespace Lucy {

	class EditorApplication : public Application
	{
	public:
		EditorApplication(const ApplicationArgs& args, const ApplicationSpecification& specs = {"LucyEditor", 1366, 768});
		~EditorApplication() = default;
		
		void OnRun() const override;

	private:
		void OnEvent() const override;
		bool m_Running;
	};

	//Creates an application with parameters
	Application* CreateEditorApplication(const ApplicationArgs& args, const ApplicationSpecification& specs) {
		return new EditorApplication(args, specs);
	}

	//Creates an application with default parameters
	Application* CreateEditorApplication(const ApplicationArgs& args) {
		return new EditorApplication(args);
	}
}