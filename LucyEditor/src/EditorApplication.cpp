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
		m_ModuleStack.Push(CreateRef<EditorModule>(m_Window));

		LUCY_ASSERT(NFD_Init() == NFD_OKAY);
	}

	EditorApplication::~EditorApplication() {
		m_Running = false;
		for (RefLucy<Module> m : m_ModuleStack.GetStack()) {
			m->Destroy();
		}
		Renderer::Destroy();
		m_Window->Destroy();
		NFD_Quit();
	}

	void EditorApplication::Run() {
		while (m_Running && !glfwWindowShouldClose(m_Window->Raw())) {

			m_Window->PollEvents();
			for (RefLucy<Module> mod : m_ModuleStack.GetStack()) {
				mod->Begin(s_Metrics);
				mod->OnRender();
				mod->End();
			}
			s_Metrics.Update();
		}
	}

	void EditorApplication::OnEvent(Event* e) {
		m_Window->WaitEventsIfMinimized();
		for (RefLucy<Module> mod : m_ModuleStack.GetStack()) {
			mod->OnEvent(*e);
		}
	}
}