#include "PropertiesPanel.h"

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

		//TODO: Temporary (change this with EntitiyContext)
		char buf[128];
		memset(buf, 'O', 127 * sizeof(char));
		buf[127] = '\0';

		if (ImGui::InputText("##hidelabel EntityTagLabel", buf, 128, ImGuiInputTextFlags_EnterReturnsTrue)) {
		}

		if (ImGui::CollapsingHeader("Transforms", ImGuiTreeNodeFlags_DefaultOpen)) {
			float x = 150;
			float y = 50;
			float z = 25;

			static ImGuiTableFlags flags = ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoHostExtendX;

			if (ImGui::BeginTable("Transform Table", 2, flags)) {

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Translation");
				ImGui::SameLine();
				ImGui::TableSetColumnIndex(1);
				RenderTransformControl("Translation Control", x, y, z, 0.0f, 0.1f);
				
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Rotation");
				ImGui::SameLine();
				ImGui::TableSetColumnIndex(1);
				RenderTransformControl("Rotation Control", x, y, z, 0.0f, 0.1f);

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Scale");
				ImGui::SameLine();
				ImGui::TableSetColumnIndex(1);
				RenderTransformControl("Scale Control", x, y, z, 1.0f, 0.1f);

				ImGui::EndTable();
			}
		}

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
		if (ImGui::Button("Y", { buttonWidth, lineHeight}))
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