#include "EditorApplication.h"
#include <iostream>

namespace Lucy {

	EditorApplication::EditorApplication(const ApplicationArgs& args)
		: Application(args)
	{
		m_LayerStack.Push({ &EditorLayer::GetInstance(), &ImGuiLayer::GetInstance() });

		m_Running = true;

		WindowSpecification w_Specs;
		w_Specs.Width = 1366;
		w_Specs.Height = 766;
		w_Specs.Name = "LucyEditor - Windows x64";
		w_Specs.DoubleBuffered = true;
		w_Specs.Resizable = true;
		w_Specs.VSync = false;

		m_Window = Window::Create(w_Specs);

		m_Window->Init();
		Renderer::Init(RenderContextType::OPENGL);

		ImGuiLayer::GetInstance().Init(m_Window->Raw());
	}

	EditorApplication::~EditorApplication()
	{
		m_Running = false;

		m_Window->Destroy();
		Renderer::Destroy();
	}

	void EditorApplication::Run()
	{
		auto& rendererAPI = Renderer::GetRendererAPI();

		while (m_Running && !glfwWindowShouldClose(m_Window->Raw())) {

			for (Layer* layer : m_LayerStack.GetStack()) {

				for (Event* event : EventDispatcher::GetInstance().GetEventPool())
					layer->OnEvent(*event);

				layer->Begin();
				layer->OnRender();
				layer->End();
			}

			rendererAPI->SwapBuffers(m_Window->Raw());
			m_Window->PollEvents();

			GLenum state = glGetError();
			if (state != GL_NO_ERROR) Logger::Log(LoggerInfo::LUCY_CRITICAL, state);
		}
	}
}