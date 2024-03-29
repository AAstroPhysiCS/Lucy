#include "EditorModule.h"

#include "Renderer/RendererModule.h"

namespace Lucy {

	EditorModule::EditorModule(Ref<Window> window, Ref<Scene> scene, Ref<RendererModule> rendererModule)
		: Module(window, scene) {
		m_ImGuiOverlay.Init(m_Window, m_Scene, rendererModule);
	}

	void EditorModule::Begin() {
		LUCY_PROFILE_NEW_EVENT("EditorModule::Begin");
	}

	void EditorModule::OnRender() {
		LUCY_PROFILE_NEW_EVENT("EditorModule::OnRender");

		m_ImGuiOverlay.Render();
		m_ImGuiOverlay.SendImGuiDataToDevice();
	}

	void EditorModule::End() {
		LUCY_PROFILE_NEW_EVENT("EditorModule::End");
	}

	void EditorModule::OnEvent(Event& e) {
		m_ImGuiOverlay.OnEvent(e);
	}

	void EditorModule::Destroy() {
		m_ImGuiOverlay.Destroy();
	}

	void EditorModule::Wait() {
		LUCY_PROFILE_NEW_EVENT("EditorModule::Wait");
		//Empty
	}
}