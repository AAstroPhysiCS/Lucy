#include "lypch.h"
#include "EditorApplication.h"

#include "Events/EventDispatcher.h"
#include "Events/KeyCodes.h"
#include "Events/WindowEvent.h"
#include "Events/InputEvent.h"

#include "EditorModule.h"
#include "Renderer/RendererModule.h"

namespace Lucy {

	Application* CreateEditorApplication(const ApplicationArgs& args, const ApplicationCreateInfo& applicationCreateInfo) {
		return new EditorApplication(args, applicationCreateInfo);
	}

	EditorApplication::EditorApplication(const ApplicationArgs& args, const ApplicationCreateInfo& applicationCreateInfo)
		: Application(args, applicationCreateInfo) {
		m_Scene = Memory::CreateRef<Scene>();

		Ref<RendererModule> rendererModule = Memory::CreateRef<RendererModule>(applicationCreateInfo.RenderArchitecture, m_Window, m_Scene);
		Ref<EditorModule> editorModule = Memory::CreateRef<EditorModule>(m_Window, m_Scene);

		m_ModuleStack.Push(editorModule);
		m_ModuleStack.Push(rendererModule);
	}

	EditorApplication::~EditorApplication() {
		for (Ref<Module> m : m_ModuleStack.GetStack())
			m->Wait();
		m_Scene->Destroy();
	}

	void EditorApplication::Run() {
		while (!glfwWindowShouldClose(m_Window->Raw())) {
			m_Window->PollEvents();

			/*
			* All the modules needs to pass the corresponding module stage in order to switch to the next stage
			*/
			//so, all the modules needs to pass the funcs, step by step
			for (Ref<Module> mod : m_ModuleStack.GetStack())
				mod->Begin();
			for (Ref<Module> mod : m_ModuleStack.GetStack())
				mod->OnRender();
			for (Ref<Module> mod : m_ModuleStack.GetStack())
				mod->End();
			s_Metrics.Update();
		}
	}

	void EditorApplication::OnEvent(Event* e) {
		m_Window->WaitEventsIfMinimized();

		EventDispatcher& dispatcher = EventDispatcher::GetInstance();
		dispatcher.Dispatch<KeyEvent>(*e, EventType::KeyEvent, [&](const KeyEvent& e) {
			if (e == KeyCode::Escape) {
				glfwSetWindowShouldClose(m_Window->Raw(), true);
			}
		});

		for (Ref<Module> mod : m_ModuleStack.GetStack()) {
			mod->OnEvent(*e);
		}
	}
}