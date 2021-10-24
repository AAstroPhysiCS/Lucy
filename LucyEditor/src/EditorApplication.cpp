#include <iostream>

#include "EditorApplication.h"

#include "EditorLayer.h"
#include "ImGuiLayer.h"

#include "Renderer/Renderer.h"
#include "Renderer/RenderCommand.h"

#include "glad/glad.h"

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
		m_Window->SetEventCallback(std::bind(&EditorApplication::OnEvent, this, std::placeholders::_1));

		Renderer::Init(RenderContextType::OpenGL);
		EditorLayer::GetInstance().Init(m_Window->Raw());
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
		while (m_Running && !glfwWindowShouldClose(m_Window->Raw())) {

			for (Layer* layer : m_LayerStack.GetStack()) {
				layer->Begin();
				layer->OnRender();
				layer->End();
			}

			RenderCommand::SwapBuffers(m_Window->Raw());
			m_Window->PollEvents();
		}
	}

	void EditorApplication::OnEvent(Event* e)
	{
		for (Layer* layer : m_LayerStack.GetStack()) {
			layer->OnEvent(*e);
		}
	}
}