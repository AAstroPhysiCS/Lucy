#include "SceneHierarchyPanel.h"

#include "imgui.h"

#include "Scene/Entity.h"
#include "Renderer/Renderer.h"

namespace Lucy {
	
	SceneHierarchyPanel& SceneHierarchyPanel::GetInstance()
	{
		static SceneHierarchyPanel s_Instance;
		return s_Instance;
	}
	
	void SceneHierarchyPanel::Render()
	{

		ImGui::Begin("Scene Hierarchy");

		auto& scene = Renderer::GetActiveScene();
		auto view = scene.registry.view<UUIDComponent, TagComponent>();
		
		for (auto entity : view) {
			TagComponent& tag = view.get<TagComponent>(entity);
			std::string& name = tag.GetTag();

			UUIDComponent& uuidComponent = view.get<UUIDComponent>(entity);
			std::string& uuid = uuidComponent.GetUUID();
		
			ImGui::Text(name.c_str());
			ImGui::Text(uuid.c_str());
		}

		if (ImGui::BeginPopupContextWindow("Entity menu")) {
			
			if (ImGui::Button("Create Entity")) {
				Entity e = Renderer::GetActiveScene().CreateEntity();
			}
			ImGui::Separator();
			
			ImGui::EndPopup();
		}

		ImGui::End();
	}
}
