#include "SceneExplorerPanel.h"
#include "ViewportPanel.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	SceneExplorerPanel& SceneExplorerPanel::GetInstance() {
		static SceneExplorerPanel s_Instance;
		return s_Instance;
	}

	void SceneExplorerPanel::OnEvent(Event& e) {
		EventDispatcher& dispatcher = EventDispatcher::GetInstance();

		dispatcher.Dispatch<MouseEvent>(e, EventType::MouseEvent, [&](const MouseEvent& e) {
			ViewportPanel& viewportPanel = ViewportPanel::GetInstance();
			if (viewportPanel.IsOverAnyGizmo() || !viewportPanel.IsViewportActive()) return;

			if (e == MouseCode::Button0) {
				Entity e = Renderer::OnMousePicking(m_IDPipeline);
				e.IsValid() ? SetEntityContext(e) : SetEntityContext({});
			}
		});
	}

	void SceneExplorerPanel::SetEntityContext(Entity e) {
		m_EntityContext = e;
	}

	void SceneExplorerPanel::SetIDPipeline(Ref<Pipeline> pipeline) {
		m_IDPipeline = pipeline;
	}

	void SceneExplorerPanel::SetScene(Ref<Scene> scene) {
		m_Scene = scene;
	}

	void SceneExplorerPanel::Render() {
		ImGui::Begin("Scene Explorer", 0, ImGuiWindowFlags_NoBringToFrontOnFocus);

		const auto& view = m_Scene->View<UUIDComponent, TagComponent>();

		uint32_t id = 0;
		for (auto entity : view) {
			Entity e = { m_Scene.Get(), entity};
			TagComponent& tag = e.GetComponent<TagComponent>();
			std::string& name = tag.GetTag();

			ImGui::PushID(id);
			if (ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_OpenOnArrow)) {
				//parenting
				ImGui::TreePop();
			}
			if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
				m_EntityContext = e;
			}
			ImGui::PopID();

			id++;
		}

		if (ImGui::BeginPopupContextWindow("Entity menu")) {

			if (ImGui::Button("Create Entity"))
				m_Scene->CreateEntity();
			ImGui::Separator();
			if (ImGui::Button("Create Mesh"))
				m_Scene->CreateMesh();

			ImGui::EndPopup();
		}

		ImGui::End();
	}
}
