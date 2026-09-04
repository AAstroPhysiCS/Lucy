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
		static bool pOpen = true;

		ImGui::Begin("Renderer Settings", &pOpen, ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse);
		auto& settings = Renderer::GetRendererSettings();

		ImGui::SliderFloat("Environment LOD", &settings.EnvironmentLOD, 0.0f, PrefilterPass::MAX_MIP_LEVELS);
		ImGui::DragFloat("Environment Intensity", &settings.EnvironmentIntensity, 0.01f, 0.0f, 10.0f, "%.3f");
		ImGui::SliderFloat("DDGI Strength", &settings.DDGIStrength, 1.0f, 5.0f);

		ImGui::SeparatorText("GPU Culling");

		const auto DrawCullFlag = [&](const char* label, GPUCullViewFlags flag) {
			const uint32_t flagValue = static_cast<uint32_t>(flag);
			bool enabled = (settings.CullViewFlags & flagValue) != 0;

			if (ImGui::Checkbox(label, &enabled)) {
				if (enabled)
					settings.CullViewFlags |= flagValue;
				else
					settings.CullViewFlags &= ~flagValue;
			}
		};

		DrawCullFlag("Frustum Culling", GPUCullViewFlags::EnableFrustumCulling);
		DrawCullFlag("Cone Culling", GPUCullViewFlags::EnableConeCulling);
		DrawCullFlag("Occlusion Culling", GPUCullViewFlags::EnableOcclusionCulling);
		DrawCullFlag("Freeze Culling View", GPUCullViewFlags::CameraFreeze);

		if ((settings.CullViewFlags & static_cast<uint32_t>(GPUCullViewFlags::CameraFreeze)) != 0)
			ImGui::TextColored({ 1.0f, 0.7f, 0.0f, 1.0f }, "Culling camera is frozen");

		ImGui::Text("Flags: 0x%08X", settings.CullViewFlags);

		ImGui::Checkbox("Show probe spheres", &settings.ShowProbeSpheres);

		ImGui::SeparatorText("Shaders");

		const auto& shaders = Renderer::GetShaderLibrary();

		for (const auto& name : shaders | std::views::keys) {
			const char* nameCStr = name.c_str();

			ImGui::PushID(nameCStr);
			ImGui::TextUnformatted(nameCStr);
			ImGui::SameLine();

			if (ImGui::Button("Reload", { 50, 17 }))
				Renderer::ReloadShader(name);

			ImGui::PopID();
		}

		ImGui::End();
	}
}