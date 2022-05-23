#include "lypch.h"
#include "EditorApplication.h"

#include "Core/Window.h"

#include "EditorModule.h"
#include "Renderer/Renderer.h"

#include "../nativefiledialog/include/nfd.h"
#include "glad/glad.h"

namespace Lucy {

	PerformanceMetrics EditorApplication::s_Metrics;

	Application* CreateEditorApplication(const ApplicationArgs& args, ApplicationSpecification& specs) {
		return new EditorApplication(args, specs);
	}

	EditorApplication::EditorApplication(const ApplicationArgs& args, ApplicationSpecification& specs)
		: Application(args, specs) {
		m_ModuleStack.Push({ &EditorModule::GetInstance() });
		m_Running = true;

		RendererSpecification rendererSpecs;
		rendererSpecs.Architecture = RenderArchitecture::Vulkan;

		specs.WindowSpecification.Name = fmt::format("{0} - Windows x64 {1}", specs.WindowSpecification.Name, 
													 rendererSpecs.Architecture == RenderArchitecture::OpenGL ? "OpenGL" : "Vulkan");

		m_Window = Window::Create(specs.WindowSpecification);
		m_Window->Init(rendererSpecs.Architecture);
		m_Window->SetEventCallback(LUCY_BIND_FUNC(&EditorApplication::OnEvent));
		
		rendererSpecs.Window = m_Window;

		Renderer::Init(rendererSpecs);
		EditorModule::GetInstance().Init(m_Window);

		LUCY_ASSERT(NFD_Init() == NFD_OKAY);
	}

	EditorApplication::~EditorApplication() {
		m_Running = false;
		for (Module* layer : m_ModuleStack.GetStack()) {
			layer->Destroy();
		}
		Renderer::Destroy();
		m_Window->Destroy();
		NFD_Quit();
	}

	void EditorApplication::Run() {
		while (m_Running && !glfwWindowShouldClose(m_Window->Raw())) {

			for (Module* layer : m_ModuleStack.GetStack()) {
				layer->Begin(s_Metrics);
				layer->OnRender();
				layer->End();
			}
			s_Metrics.Update();
			m_Window->PollEvents();
		}
	}

	void EditorApplication::OnEvent(Event* e) {
		for (Module* layer : m_ModuleStack.GetStack()) {
			layer->OnEvent(*e);
		}
	}
}