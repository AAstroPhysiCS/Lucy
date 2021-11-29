#include "PerformancePanel.h"

#include "imgui.h"
#include "../EditorApplication.h"

namespace Lucy {
	
	PerformancePanel& PerformancePanel::GetInstance() {
		static PerformancePanel s_Instance;
		return s_Instance;
	}

	void PerformancePanel::SetShow(bool show) {
		m_Show = show;
	}

	void PerformancePanel::Render() {
		if (!m_Show) return;

		static bool pOpen = false;
		ImGui::Begin("Performance", &pOpen, ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse);

		PerformanceMetrics& metrics = EditorApplication::GetPerformanceMetrics();
		ImGui::Text(fmt::format("Frame Time: {0} ms", metrics.FrameTime).c_str());
		ImGui::Text(fmt::format("Frames: {0} FPS", metrics.Frames).c_str());
		//ImGui::Text(fmt::format("Delta Time: {0} ms", metrics.DeltaTime).c_str());

		ImGui::End();
	}
}