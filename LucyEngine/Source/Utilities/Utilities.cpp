#include "lypch.h"

#include "Utilities.h"
#include "nfd.h"

#include "Core/FileSystem.h"

#include "../ImGui/imgui.h"

namespace Utils {

	std::vector<std::string> Split(const std::string& s, const std::string& delimiter) {
		size_t start = 0;
		size_t end = 0;
		std::string token;
		std::vector<std::string> buffer;

		while ((end = s.find(delimiter, start)) != std::string::npos) {
			token = s.substr(start, end - start);
			start = end + delimiter.length();
			buffer.push_back(token);
		}

		buffer.push_back(s.substr(start));
		return buffer;
	}

	Attribute ReadAttributeFromIni(const char* windowName, const char* attributeName) {
		std::string buffer;
		Lucy::FileSystem::ReadFile("lucyconfig.ini", buffer);

		std::string windowNameFull = "[Window][";
		windowNameFull.append(windowName).append("]");

		std::size_t windowIndex = buffer.find(windowName);
		std::size_t attribIndex = buffer.find(attributeName, windowIndex);

		std::size_t endOfChar = buffer.find("\n", attribIndex);

		std::string attrib = buffer.substr(attribIndex, endOfChar - attribIndex);
		const std::vector<std::string>& vec = Split(Split(attrib, "=")[1], ",");

		Attribute sizeObject{ std::stoul(vec[0].c_str()), std::stoul(vec[1].c_str()) };
		return sizeObject;
	}

	void OpenDialog(std::string& outString, const DialogFilter filterList[], size_t count, const char* defaultPath) {
		char* outPath;
		nfdresult_t result = NFD_OpenDialog(&outPath, (nfdfilteritem_t*)filterList, (nfdfiltersize_t)count, defaultPath);
		if (result == NFD_OKAY) {
			outString = std::string(outPath);
			NFD_FreePath((nfdu8char_t*)outPath);
		}
	}
}

namespace UI {

	void TextCenter(const std::string& text, float xPadding, uint32_t indent) {
		TextCenter(text.c_str(), xPadding);
	}

	void TextCenter(const char* const text, float xPadding, uint32_t indent) {
		float fontSize = ImGui::GetFontSize() * strlen(text) / 2;
		ImGui::SameLine(ImGui::GetWindowSize().x / 2 - fontSize + (fontSize / 2));
		ImGui::Text(text);
	}

	void TextCenterTable(const char* const text, float xPadding, float yPadding, uint32_t indent) {
		float fontSize = ImGui::GetFontSize() * strlen(text) / 2;
		float yHalfed = ImGui::CalcItemWidth() / ImGui::GetColumnsCount() / 2;
		ImGui::SetCursorPosX(ImGui::GetColumnWidth() / 2 - fontSize + (fontSize / 2) + xPadding + indent);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + yHalfed + yPadding);
		ImGui::Text(text);
	}

	void TransformControl(const char* id, float& x, float& y, float& z, float defaultValue, float speed) {
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

namespace Maths {

	//From: https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection
	bool RayTriangleIntersection(const Ray& r, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, float& t, float& u, float& v) {
		glm::vec3 v0v1 = v1 - v0;
		glm::vec3 v0v2 = v2 - v0;
		glm::vec3 pvec = glm::cross(r.Dir, v0v2);
		float det = glm::dot(v0v1, pvec);
		// if the determinant is negative the triangle is backfacing
		// if the determinant is close to 0, the ray misses the triangle
		if (det < 0.0001f) return false;
		// ray and triangle are parallel if det is close to 0
		if (fabs(det) < 0.0001f) return false;
		float invDet = 1 / det;

		glm::vec3 tvec = r.Origin - v0;
		u = glm::dot(tvec, pvec) * invDet;
		if (u < 0 || u > 1) return false;

		glm::vec3 qvec = glm::cross(tvec, v0v1);
		v = glm::dot(r.Dir, qvec) * invDet;
		if (v < 0 || u + v > 1) return false;

		t = glm::dot(v0v2, qvec) * invDet;

		return true;
	}
}