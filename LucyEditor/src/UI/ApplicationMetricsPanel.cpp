#include "ApplicationMetricsPanel.h"

#include "imgui.h"
#include "../EditorApplication.h"

namespace Lucy {
	
	ApplicationMetricsPanel& ApplicationMetricsPanel::GetInstance() {
		static ApplicationMetricsPanel s_Instance;
		return s_Instance;
	}

	void ApplicationMetricsPanel::SetShow(bool show) {
		m_Show = show;
	}

	void ApplicationMetricsPanel::Render() {
		Metrics& metrics = EditorApplication::GetApplicationMetrics();

		PerformanceMetrics& perfMetrics = metrics.PerfMetrics;
		MemoryTracker& tracker = metrics.MemTracker;

		if (!m_Show) return;

		static bool pOpen = false;
		ImGui::Begin("Application Metrics", &pOpen, ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse);

		ImGui::Text(fmt::format("Frame Time: {0} ms", perfMetrics.FrameTime).c_str());
		ImGui::Text(fmt::format("Frames: {0} FPS", perfMetrics.Frames).c_str());
		//ImGui::Text(fmt::format("Delta Time: {0} ms", metrics.DeltaTime).c_str());

		ImGui::Separator();

		ImGui::Text(fmt::format("Total Allocated Memory: {0} mb", tracker.GetTotalAllocated()).c_str());
		ImGui::Text(fmt::format("Total Freed Memory: {0} mb", tracker.GetTotalFreed()).c_str());
		ImGui::Text(fmt::format("Current Usage: {0} mb", tracker.GetCurrentUsage()).c_str());

		ImGui::End();
	}
}