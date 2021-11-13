#include <iostream>

#include "EditorApplication.h"

#include "EditorLayer.h"
#include "ImGuiLayer.h"

#include "Renderer/Renderer.h"
#include "Renderer/RenderCommand.h"

#include "../nativefiledialog/include/nfd.h"
#include "glad/glad.h"

namespace Lucy {

	EditorApplication::EditorApplication(const ApplicationArgs& args)
		: Application(args) {
		m_LayerStack.Push({ &ImGuiLayer::GetInstance(), &EditorLayer::GetInstance() });

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

		Renderer::Init(m_Window, RenderAPI::OpenGL);
		EditorLayer::GetInstance().Init(m_Window);
		ImGuiLayer::GetInstance().Init(m_Window);

		LUCY_ASSERT(NFD_Init() == NFD_OKAY);
	}

	EditorApplication::~EditorApplication() {
		m_Running = false;
		//layers are being destroyed in the base class
		m_Window->Destroy();
		NFD_Quit();
	}

	void EditorApplication::Run() {
		while (m_Running && !glfwWindowShouldClose(m_Window->Raw())) {
			for (Layer* layer : m_LayerStack.GetStack()) {
				layer->Begin();
				layer->OnRender();
				layer->End();
			}
			m_Window->PollEvents();
		}
	}

	void EditorApplication::OnEvent(Event* e) {
		for (Layer* layer : m_LayerStack.GetStack()) {
			layer->OnEvent(*e);
		}
	}
}