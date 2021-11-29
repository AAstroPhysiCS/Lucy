#include "lypch.h"
#include "UIUtils.h"

#include "../ImGui/imgui.h"

namespace Lucy {

	void UIUtils::TextCenter(const std::string& text, float xPadding, uint32_t indent) {
		TextCenter(text, xPadding);
	}

	void UIUtils::TextCenter(const char const* text, float xPadding, uint32_t indent) {
		float fontSize = ImGui::GetFontSize() * strlen(text) / 2;
		ImGui::SameLine(ImGui::GetWindowSize().x / 2 - fontSize + (fontSize / 2));
		ImGui::Text(text);
	}

	void UIUtils::TextCenterTable(const char const* text, float xPadding, float yPadding, uint32_t indent) {
		float fontSize = ImGui::GetFontSize() * strlen(text) / 2;
		float yHalfed = ImGui::CalcItemWidth() / ImGui::GetColumnsCount() / 2;
		ImGui::SetCursorPosX(ImGui::GetColumnWidth() / 2 - fontSize + (fontSize / 2) + xPadding + indent);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + yHalfed + yPadding);
		ImGui::Text(text);
	}

	void UIUtils::TransformControl(const char* id, float& x, float& y, float& z, float defaultValue, float speed) {
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
