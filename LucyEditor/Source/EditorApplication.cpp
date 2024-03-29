#include "lypch.h"
#include "EditorApplication.h"

#include "Events/KeyCodes.h"
#include "Events/WindowEvent.h"
#include "Events/InputEvent.h"

#include "EditorModule.h"
#include "Renderer/RendererModule.h"

namespace Lucy {

	Application* CreateApplication(const ApplicationArgs& args, const ApplicationCreateInfo& applicationCreateInfo) {
		auto& instance = Application::Get();
		instance = new EditorApplication(args, applicationCreateInfo);
		return instance;
	}
}

EditorApplication::EditorApplication(const ApplicationArgs& args, const ApplicationCreateInfo& applicationCreateInfo)
	: Application(args, applicationCreateInfo) {
	m_Scene = Memory::CreateRef<Scene>();

	Ref<RendererModule> rendererModule = Memory::CreateRef<RendererModule>(applicationCreateInfo.RenderArchitecture, m_Window, m_Scene);
	Ref<EditorModule> editorModule = Memory::CreateRef<EditorModule>(m_Window, m_Scene, rendererModule);

	m_ModuleStack.Push(editorModule);
	m_ModuleStack.Push(rendererModule);
}

EditorApplication::~EditorApplication() {
	for (Ref<Module> mod : m_ModuleStack)
		mod->Wait();
	m_Scene->Destroy();
}

void EditorApplication::Run() {
	while (!glfwWindowShouldClose(m_Window->Raw())) {
		LUCY_PROFILE_NEW_FRAME("Lucy");

		m_Window->PollEvents();

		/*
		* All the modules needs to pass the corresponding module stage in order to switch to the next stage
		* so, all the modules needs to pass the funcs, step by step
		*/
		LUCY_PROFILE_NEW_EVENT("Module::Begin");
		for (Ref<Module> mod : m_ModuleStack)
			mod->Begin();
		LUCY_PROFILE_NEW_EVENT("Module::OnRender");
		for (Ref<Module> mod : m_ModuleStack)
			mod->OnRender();
		LUCY_PROFILE_NEW_EVENT("Module::End");
		for (Ref<Module> mod : m_ModuleStack)
			mod->End();
		LUCY_PROFILE_NEW_EVENT("Metrics::Update");
		s_Metrics.Update();
	}
}

void EditorApplication::OnEvent(Event* e) {
	m_Window->WaitEventsIfMinimized();

	m_InputHandler.Dispatch<KeyEvent>(*e, EventType::KeyEvent, [&](const KeyEvent& e) {
		if (e == KeyCode::Escape) {
			glfwSetWindowShouldClose(m_Window->Raw(), true);
		}
	});

	for (Ref<Module> mod : m_ModuleStack)
		mod->OnEvent(*e);
}