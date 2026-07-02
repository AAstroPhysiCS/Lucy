#include <ranges>

#include "RendererSettingsPanel.h"
#include "Renderer/Renderer.h"
#include "Renderer/RendererPasses.h"

#include "imgui.h"

namespace Lucy {
	
	RendererSettingsPanel& RendererSettingsPanel::GetInstance() {
		static RendererSettingsPanel s_Instance;
		return s_Instance;
	}

	void RendererSettingsPanel::Render() {
		static bool pOpen = false;
		ImGui::Begin("Renderer Settings", &pOpen, ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse);

		auto& settings = Renderer::GetRendererSettings();
		ImGui::SliderFloat("Environment LOD", &settings.EnvironmentLOD, 0.0f, PrefilterPass::MAX_MIP_LEVELS);

		const auto& shaders = Renderer::GetShaderLibrary();
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