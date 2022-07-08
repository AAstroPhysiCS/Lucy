#include "lypch.h"
#include "EditorApplication.h"

#include "EditorModule.h"

namespace Lucy {

	Application* CreateEditorApplication(const ApplicationArgs& args, const ApplicationCreateInfo& applicationCreateInfo) {
		return new EditorApplication(args, applicationCreateInfo);
	}

	EditorApplication::EditorApplication(const ApplicationArgs& args, const ApplicationCreateInfo& applicationCreateInfo)
		: Application(args, applicationCreateInfo) {
		m_ModuleStack.Push(Memory::CreateRef<EditorModule>(m_Window));
	}

	void EditorApplication::Run() {
		while (!glfwWindowShouldClose(m_Window->Raw())) {

			m_Window->PollEvents();
			for (Ref<Module> mod : m_ModuleStack.GetStack()) {
				mod->Begin();
				mod->OnRender();
				mod->End();
			}
			s_Metrics.Update();
		}
	}

	void EditorApplication::OnEvent(Event* e) {
		m_Window->WaitEventsIfMinimized();
		for (Ref<Module> mod : m_ModuleStack.GetStack()) {
			mod->OnEvent(*e);
		}
	}
}