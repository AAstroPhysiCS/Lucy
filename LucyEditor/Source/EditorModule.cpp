#include "EditorModule.h"

namespace Lucy {

	EditorModule::EditorModule(Ref<Window> window, Ref<Scene> scene)
		: Module(window, scene) {
		m_ImGuiOverlay.Init(m_Window, m_Scene);
	}

	void EditorModule::Begin() {

	}

	void EditorModule::OnRender() {
		m_ImGuiOverlay.Render();
		m_ImGuiOverlay.SendImGuiDataToGPU();
	}

	void EditorModule::End() {

	}

	void EditorModule::OnEvent(Event& e) {
		m_ImGuiOverlay.OnEvent(e);
	}

	void EditorModule::Destroy() {
		m_ImGuiOverlay.Destroy();
	}
}