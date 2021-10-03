#include "EditorApplication.h"
#include <iostream>

namespace Lucy {

	EditorApplication::EditorApplication(const ApplicationArgs& args, const ApplicationSpecification& specs)
		: Application(args, specs)
	{
		m_LayerStack.Push(EditorLayer::GetInstance());
		m_Running = true;
	}

	void EditorApplication::OnRun() const
	{
		Renderer::Init();

		while (m_Running) {
			
		}
	}

	void EditorApplication::OnEvent() const
	{

	}
}
