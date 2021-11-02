#include "PropertiesPanel.h"

#include "Renderer/Renderer.h"
#include "SceneHierarchyPanel.h"
#include "Utils.h"

#include "imgui.h"

namespace Lucy {

	PropertiesPanel& PropertiesPanel::GetInstance()
	{
		static PropertiesPanel s_Instance;
		return s_Instance;
	}

	void PropertiesPanel::Render()
	{
		ImGui::Begin("Properties");

		auto& entityContext = SceneHierarchyPanel::GetInstance().GetEntityContext();
		if (!entityContext.IsValid()) {
			ImGui::End();
			return;
		}

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Name");
		ImGui::SameLine();

		char buffer[512];
		std::string& tag = entityContext.GetComponent<TagComponent>().GetTag();
		std::strncpy(buffer, tag.c_str(), sizeof(buffer));
		ImGui::InputText("##hidelabel EntityTagLabel", buffer, sizeof(buffer));
		tag = buffer;

		ImGui::SameLine(0, 20);
		if (ImGui::Button("Add Component"))
			ImGui::OpenPopup("AddComponentPopup", ImGuiPopupFlags_AnyPopup);

		if (ImGui::BeginPopup("AddComponentPopup")) {
			if (ImGui::BeginMenu("Mesh")) {
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Script")) {
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Light")) {
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Camera")) {
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Physics")) {
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("UI")) {
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Audio")) {
				ImGui::EndMenu();
			}

			ImGui::EndPopup();
		}

		//Components
		DrawComponentPanel<TransformComponent>(entityContext, [&](TransformComponent& t) {
			if (ImGui::CollapsingHeader("Transforms", ImGuiTreeNodeFlags_DefaultOpen)) {

				static ImGuiTableFlags flags = ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoHostExtendX;

				if (ImGui::BeginTable("Transform Table", 2, flags)) {

					glm::vec3& pos = t.GetPosition();
					glm::vec3& rot = t.GetRotation();
					glm::vec3& scale = t.GetScale();

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Translation");
					ImGui::SameLine();
					ImGui::TableSetColumnIndex(1);
					RenderTransformControl("Translation Control", pos.x, pos.y, pos.z, 0.0f, 0.1f);

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Rotation");
					ImGui::SameLine();
					ImGui::TableSetColumnIndex(1);
					RenderTransformControl("Rotation Control", rot.x, rot.y, rot.z, 0.0f, 0.1f);

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Scale");
					ImGui::SameLine();
					ImGui::TableSetColumnIndex(1);
					RenderTransformControl("Scale Control", scale.x, scale.y, scale.z, 1.0f, 0.1f);

					ImGui::EndTable();
				}
			}
		});

		DrawComponentPanel<MeshComponent>(entityContext, [](MeshComponent& c) {
			if(ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
				
				char buf[1024];
				memset(buf, 0, sizeof(buf));

				RefLucy<Mesh> mesh = c.GetMesh();
				if (mesh.get()) {
					std::string& path = mesh->GetPath();
					std::strncpy(buf, path.c_str(), sizeof(buf));
				}

				ImGui::AlignTextToFramePadding();
				ImGui::Text("Path");
				ImGui::SameLine();
				if (ImGui::InputText("##hideLabel MeshPath", buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue))
					c.SetMesh(Mesh::Create(buf));

				ImGui::SameLine(0, 20);
				
				if (ImGui::Button("L")) {
					std::string outPath;
					Utils::OpenDialog(outPath, Utils::MeshFilterList, 1, "assets/");
					if (!outPath.empty())
						c.SetMesh(Mesh::Create(outPath));
				}
			}
		});

		static bool demoOpen = false;
		if (ImGui::RadioButton("Demo Window", demoOpen)) demoOpen = !demoOpen;
		if (demoOpen) ImGui::ShowDemoWindow();

		ImGui::End();
	}

	void PropertiesPanel::RenderTransformControl(const char* id, float& x, float& y, float& z, float defaultValue, float speed)
	{
		ImGui::PushID(id);

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 4 });
		ImFont* font = ImGui::GetFont();
		float lineHeight = font->FontSize + ImGui::GetStyle().FramePadding.y * 2.0f;
		float buttonWidth = lineHeight + 3.0f;

		ImGui::PushStyleColor(ImGuiCol_Button, { 0.64f, 0.4f, 0.38f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.74f, 0.4f, 0.38f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.64f, 0.4f, 0.38f, 1.0f });

		if (ImGui::Button("X", { buttonWidth, lineHeight }))
			x = defaultValue;
		ImGui::PopStyleColor(3);

		ImGui::SameLine();

		ImGui::PushItemWidth(ImGui::CalcItemWidth() / 3 + 17);
		ImGui::AlignTextToFramePadding();
		ImGui::DragFloat("##hidelabel X", &x, speed);
		ImGui::PopItemWidth();

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, { 0.46f, 0.59f, 0.5f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.46f, 0.69f, 0.5f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.46f, 0.59f, 0.5f, 1.0f });
		if (ImGui::Button("Y", { buttonWidth, lineHeight }))
			y = defaultValue;
		ImGui::PopStyleColor(3);

		ImGui::SameLine();

		ImGui::PushItemWidth(ImGui::CalcItemWidth() / 3 + 17);
		ImGui::AlignTextToFramePadding();
		ImGui::DragFloat("##hidelabel Y", &y, speed);
		ImGui::PopItemWidth();

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, { 0.33f, 0.48f, 0.6f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.33f, 0.48f, 0.7f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.33f, 0.48f, 0.6f, 1.0f });
		if (ImGui::Button("Z", { buttonWidth, lineHeight }))
			z = defaultValue;
		ImGui::PopStyleColor(3);

		ImGui::SameLine();

		ImGui::PushItemWidth(ImGui::CalcItemWidth() / 3 + 17);
		ImGui::AlignTextToFramePadding();
		ImGui::DragFloat("##hidelabel Z", &z, speed);
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();
		ImGui::PopID();
	}
}