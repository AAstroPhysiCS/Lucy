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

		std::string outputStr;

		fmt::format_to(std::back_inserter(outputStr), "Frame Time: {0} ms \n", perfMetrics.FrameTime);
		fmt::format_to(std::back_inserter(outputStr), "Frames: {0} FPS \n", perfMetrics.Frames);

		fmt::format_to(std::back_inserter(outputStr), "Total Allocated Memory: {0} mb \n", tracker.GetTotalAllocated());
		fmt::format_to(std::back_inserter(outputStr), "Total Freed Memory: {0} mb \n", tracker.GetTotalFreed());
		fmt::format_to(std::back_inserter(outputStr), "Current Usage: {0} mb \n", tracker.GetCurrentUsage());

		ImGui::Text(outputStr.c_str());

		ImGui::End();
	}
}