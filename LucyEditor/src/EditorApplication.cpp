#include "EditorApplication.h"
#include <iostream>

namespace Lucy {

	EditorApplication::EditorApplication(const ApplicationArgs& args)
		: Application(args)
	{
		m_LayerStack.Push(EditorLayer::GetInstance());
		m_Running = true;

		WindowSpecification w_Specs;
		w_Specs.Width = 1366;
		w_Specs.Height = 766;
		w_Specs.Name = "LucyEditor - Windows x64";
		w_Specs.DoubleBuffered = false;
		w_Specs.Resizable = true;
		w_Specs.VSync = false;

		m_Window = Window::Create(w_Specs);

		m_Window->Init();
		Renderer::Init(RendererContext::OPENGL);
	}

	void EditorApplication::Run() const
	{
		auto& rendererAPI = Renderer::GetRendererAPI();
		
		while (m_Running) {
		
			for (Layer* layer : m_LayerStack.GetStack()) {
				layer->Update();
			}
			
			rendererAPI->SwapBuffers(m_Window->GetRaw());
			m_Window->Update();
		}
	}

}
