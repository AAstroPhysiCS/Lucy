#include <ranges>

#include "RendererSettingsPanel.h"
#include "Renderer/Renderer.h"

#include "imgui.h"

namespace Lucy {
	
	RendererSettingsPanel& RendererSettingsPanel::GetInstance() {
		static RendererSettingsPanel s_Instance;
		return s_Instance;
	}

	void RendererSettingsPanel::Render() {
		static bool pOpen = false;
		ImGui::Begin("Renderer Settings", &pOpen, ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse);

		const auto& shaders = Renderer::GetAllShaders();
		for (const auto& name : shaders | std::views::keys) {
			const char* nameCStr = name.c_str();
			ImGui::PushID(nameCStr);
			ImGui::Text(nameCStr);
			ImGui::SameLine();
			if (ImGui::Button("Reload", { 50, 17 })) {
				Renderer::ReloadShader(name);
			}
			ImGui::NewLine();
			ImGui::PopID();
		}
			
		ImGui::End();
	}
}