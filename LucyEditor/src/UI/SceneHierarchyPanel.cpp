#include "SceneHierarchyPanel.h"
#include "imgui.h"
#include "Renderer/Renderer.h"
#include "../EditorLayer.h"

namespace Lucy {

	SceneHierarchyPanel& SceneHierarchyPanel::GetInstance()
	{
		static SceneHierarchyPanel s_Instance;
		return s_Instance;
	}

	void SceneHierarchyPanel::SetEntityContext(Entity e) {
		m_EntityContext = e;
	}

	void SceneHierarchyPanel::Render()
	{
		ImGui::Begin("Scene Hierarchy", 0, ImGuiWindowFlags_NoBringToFrontOnFocus);
		
		auto& scene = EditorModule::GetInstance().GetScene();
		const auto& view = scene.View<UUIDComponent, TagComponent>();

		uint32_t id = 0;
		for (auto entity : view) {
			Entity e = { &scene, entity };
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
				scene.CreateEntity();
			ImGui::Separator();
			if (ImGui::Button("Create Mesh"))
				scene.CreateMesh();

			ImGui::EndPopup();
		}

		ImGui::End();
	}
}
