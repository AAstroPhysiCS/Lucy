#include "SceneExplorerPanel.h"
#include "ViewportPanel.h"

#include "Events/EventHandler.h"

namespace Lucy {

	SceneExplorerPanel& SceneExplorerPanel::GetInstance() {
		static SceneExplorerPanel s_Instance;
		return s_Instance;
	}

	void SceneExplorerPanel::OnEvent(Event& e) {
		EventHandler::AddListener<MouseEvent>(e, [&](const MouseEvent& e) {
			ViewportPanel& viewportPanel = ViewportPanel::GetInstance();
			if (viewportPanel.IsOverAnyGizmo() || !viewportPanel.IsViewportActive()) return;

			if (e == MouseCode::Button0) {
				Entity entity{};
				EventHandler::DispatchImmediateEvent<EntityPickedEvent>(entity, m_Scene.get(), viewportPanel.GetViewportMouseX(), viewportPanel.GetViewportMouseY());
				SetEntityContext(entity);
			}
		});
	}

	void SceneExplorerPanel::SetEntityContext(Entity e) {
		m_EntityContext = e;
	}

	void SceneExplorerPanel::SetScene(Ref<Scene> scene) {
		m_Scene = scene;
	}

	void SceneExplorerPanel::Render() {
		LUCY_PROFILE_NEW_EVENT("SceneExplorerPanel::Render");
		
		ImGui::Begin("Scene Explorer", nullptr, ImGuiWindowFlags_NoBringToFrontOnFocus);

		const auto& view = m_Scene->View<UUIDComponent, TagComponent>();

		uint32_t id = 0;
		for (const auto entity : view) {
			Entity e = { m_Scene.get(), entity};
			const TagComponent& tag = e.GetComponent<TagComponent>();
			const std::string& name = tag.GetTag();

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
