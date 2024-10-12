#include "DebugPanel.h"

#include "imgui.h"
#include "Renderer/Renderer.h"

#include "../EditorApplication.h"

namespace Lucy {
	
	DebugPanel& DebugPanel::GetInstance() {
		static DebugPanel s_Instance;
		return s_Instance;
	}

	void DebugPanel::Render() {
		LUCY_PROFILE_NEW_EVENT("DebugPanel::Render");

		static bool pOpen = false;
		ImGui::Begin("Debug Panel", &pOpen, ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse);

		if (ImGui::CollapsingHeader("Renderer Metrics", ImGuiTreeNodeFlags_DefaultOpen)) {
			
			const ApplicationMetrics& appMetrics = EditorApplication::GetApplicationMetrics();
			const auto& cmdQueueMetrics = Renderer::GetCommandQueueMetrics();

			std::string outputStr;

			for (const auto& [passName, renderTime] : cmdQueueMetrics.RenderTimeOfPasses)
				fmt::format_to(std::back_inserter(outputStr), "Render pass: {0} took {1:.3f} ms \n", passName, renderTime);
			
			/*
			const auto& allPipelineStatistics = Renderer::GetPipelineManager()->GetAllGraphicsPipelineStatistics();
			for (const auto& [pipelineName, pipelineStatistic] : allPipelineStatistics) {
				fmt::format_to(std::back_inserter(outputStr), "Pipeline: {0} \n", pipelineName);
				fmt::format_to(std::back_inserter(outputStr), "- Input assembly vertex count: {0} \n", pipelineStatistic.GetAssemblyVertexCount());
				fmt::format_to(std::back_inserter(outputStr), "- Input assembly primitives count: {0} \n", pipelineStatistic.GetAssemblyVertexCount());
				fmt::format_to(std::back_inserter(outputStr), "- Vertex shader invocations: {0} \n", pipelineStatistic.GetAssemblyVertexCount());
				fmt::format_to(std::back_inserter(outputStr), "- Clipping stage primitives processed: {0} \n", pipelineStatistic.GetAssemblyVertexCount());
				fmt::format_to(std::back_inserter(outputStr), "- Clipping stage primitives output: {0} \n", pipelineStatistic.GetAssemblyVertexCount());
				fmt::format_to(std::back_inserter(outputStr), "- Fragment shader invocations: {0} \n", pipelineStatistic.GetAssemblyVertexCount());
				fmt::format_to(std::back_inserter(outputStr), "- Tess. control shader patches: {0} \n", pipelineStatistic.GetAssemblyVertexCount());
				fmt::format_to(std::back_inserter(outputStr), "- Tess. eval. shader invocations: {0} \n", pipelineStatistic.GetAssemblyVertexCount());
				fmt::format_to(std::back_inserter(outputStr), "\n");
			}
			*/

			fmt::format_to(std::back_inserter(outputStr), "Frame Time: {0:.3f} ms \n", appMetrics.GetFrameTime());
			fmt::format_to(std::back_inserter(outputStr), "Frames: {0} FPS \n", appMetrics.GetFrames());
			fmt::format_to(std::back_inserter(outputStr), "Render Time: {0:.3f} ms \n", cmdQueueMetrics.RenderTime);

			fmt::format_to(std::back_inserter(outputStr), "Total Allocated Memory: {0:.3f} mb \n", appMetrics.GetTotalAllocated());
			fmt::format_to(std::back_inserter(outputStr), "Total Freed Memory: {0:.3f} mb \n", appMetrics.GetTotalFreed());
			fmt::format_to(std::back_inserter(outputStr), "Current Usage: {0:.3f} mb \n", appMetrics.GetCurrentUsage());

			ImGui::Text(outputStr.c_str());
		}

		if (ImGui::CollapsingHeader("Memory Viewer", ImGuiTreeNodeFlags_DefaultOpen)) {
			
		}

		ImGui::End();
	}
}