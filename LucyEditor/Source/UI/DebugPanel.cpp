#include "DebugPanel.h"

#include "imgui.h"
#include "Renderer/Renderer.h"

#include "Renderer/Pipeline/GraphicsPipeline.h"

#include "../EditorApplication.h"

namespace Lucy {

	DebugPanel& DebugPanel::GetInstance() {
		static DebugPanel s_Instance;
		return s_Instance;
	}

	void DebugPanel::Render() {
		LUCY_PROFILE_NEW_EVENT("DebugPanel::Render");

		static bool pOpen = true;
		static float uiScale = ImGui::GetWindowDpiScale();

		ImGui::PushStyleColor(ImGuiCol_WindowBg, Theme::BackgroundColor);
		ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::ChildBackgroundColor);
		ImGui::PushStyleColor(ImGuiCol_Header, Theme::HeaderColor);
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, Theme::HeaderHoveredColor);
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, Theme::HeaderHoveredColor);
		ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, Theme::HeaderColor);
		ImGui::PushStyleColor(ImGuiCol_FrameBg, Theme::ChildBackgroundColor);
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, Theme::HeaderHoveredColor);
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, Theme::HeaderHoveredColor);
		ImGui::PushStyleColor(ImGuiCol_CheckMark, Theme::AccentColor);
		ImGui::PushStyleColor(ImGuiCol_SliderGrab, Theme::AccentColor);
		ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, Theme::AccentColor);
		ImGui::PushStyleColor(ImGuiCol_Separator, Theme::HeaderHoveredColor);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f * uiScale, 14.0f * uiScale));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f * uiScale, 5.0f * uiScale));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f * uiScale, 7.0f * uiScale));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f * uiScale);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f * uiScale);

		if (!ImGui::Begin("Debug Panel", &pOpen, ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse)) {
			ImGui::End();

			ImGui::PopStyleVar(5);
			ImGui::PopStyleColor(13);
			return;
		}

		ImGui::SetWindowFontScale(uiScale);

		const ApplicationMetrics& appMetrics = EditorApplication::GetApplicationMetrics();
		const auto& cmdQueueMetrics = Renderer::GetCommandQueueMetrics();

		float frameTime = static_cast<float>(appMetrics.GetFrameTime());
		float renderTime = static_cast<float>(cmdQueueMetrics.Time);

		ImVec4 frameTimeColor = frameTime < 16.67f ? Theme::GreenColor : frameTime < 33.33f ? Theme::YellowColor : Theme::RedColor;
		ImVec4 renderTimeColor = renderTime < 8.0f ? Theme::GreenColor : renderTime < 16.67f ? Theme::YellowColor : Theme::RedColor;

		if (ImGui::BeginTable("DebugSummary", 4, ImGuiTableFlags_SizingStretchSame)) {
			const auto DrawMetricCard = [&](const char* title, const std::string& value, const ImVec4& color) {
				ImGui::TableNextColumn();

				ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::ChildBackgroundColor);

				ImGui::BeginChild(title, ImVec2(0.0f, 65.0f * uiScale), true);

				ImGui::TextDisabled("%s", title);
				ImGui::Spacing();
				ImGui::TextColored(color, "%s", value.c_str());

				ImGui::EndChild();

				ImGui::PopStyleColor();
			};

			DrawMetricCard("Frame Time", std::format("{:.3f} ms", frameTime), frameTimeColor);
			DrawMetricCard("Render time", std::format("{:.3f} ms", renderTime), renderTimeColor);
			DrawMetricCard("Frame Rate", std::format("{} FPS", appMetrics.GetFrames()), Theme::AccentColor);
			DrawMetricCard("Memory", std::format("{:.2f} MB", appMetrics.GetCurrentUsage()), Theme::AccentColor);

			ImGui::EndTable();
		}

		ImGui::Spacing();

		static std::array<float, 180> frameTimeHistory{};
		static uint32_t frameTimeHistoryOffset = 0;

		frameTimeHistory[frameTimeHistoryOffset] = frameTime;
		frameTimeHistoryOffset = (frameTimeHistoryOffset + 1) % static_cast<uint32_t>(frameTimeHistory.size());

		if (ImGui::CollapsingHeader("Frame Performance", ImGuiTreeNodeFlags_DefaultOpen)) {
			float maximumFrameTime = 16.67f;
			for (float time : frameTimeHistory)
				maximumFrameTime = std::max(maximumFrameTime, time);
			maximumFrameTime *= 1.15f;

			std::string overlay = std::format("{:.2f} ms | {:.0f} FPS", frameTime, frameTime > 0.0f ? 1000.0f / frameTime : 0.0f);
			ImGui::PlotLines("##FrameTimeHistory", frameTimeHistory.data(), static_cast<int>(frameTimeHistory.size()), static_cast<int>(frameTimeHistoryOffset), 
				overlay.c_str(), 0.0f, maximumFrameTime, ImVec2(ImGui::GetContentRegionAvail().x, 110.0f * uiScale));

			float sixtyFPSBudget = std::clamp(frameTime / 16.67f, 0.0f, 1.0f);
			ImGui::TextDisabled("16.67 ms frame budget");
			ImGui::ProgressBar(sixtyFPSBudget, ImVec2(-1.0f, 6.0f * uiScale), "");
		}

		if (ImGui::CollapsingHeader("Render Passes", ImGuiTreeNodeFlags_DefaultOpen)) {
			struct RenderPassMetric {
				std::string Name;
				float Time;
			};

			std::vector<RenderPassMetric> passMetrics;
			passMetrics.reserve(cmdQueueMetrics.TimeOfPasses.size());

			for (const auto& [passName, passTime] : cmdQueueMetrics.TimeOfPasses) {
				passMetrics.push_back({
					.Name = passName,
					.Time = static_cast<float>(passTime)
				});
			}

			static char filter[128]{};

			ImGui::SetNextItemWidth(260.0f * uiScale);
			ImGui::InputTextWithHint("##RenderPassFilter", "Filter render passes...", filter, sizeof(filter));

			ImGui::Spacing();

			const ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | 
				ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp;

			if (ImGui::BeginTable("RenderPassMetrics", 3, tableFlags, ImVec2(0.0f, 320.0f * uiScale))) {
				ImGui::TableSetupColumn("Pass", ImGuiTableColumnFlags_WidthStretch, 0.0f, 0);
				ImGui::TableSetupColumn("Render Time", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortDescending | ImGuiTableColumnFlags_WidthFixed, 130.0f * uiScale, 1);
				ImGui::TableSetupColumn("Frame Share", ImGuiTableColumnFlags_WidthFixed, 110.0f * uiScale, 2);

				ImGui::TableHeadersRow();

				if (const ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs()) {
					if (sortSpecs->SpecsCount > 0) {
						const ImGuiTableColumnSortSpecs& sortSpec = sortSpecs->Specs[0];

						std::stable_sort(passMetrics.begin(), passMetrics.end(), [&](const RenderPassMetric& lhs, const RenderPassMetric& rhs) {
							const bool descending = sortSpec.SortDirection == ImGuiSortDirection_Descending;

							switch (sortSpec.ColumnUserID) {
								case 0:
									return descending ? lhs.Name > rhs.Name : lhs.Name < rhs.Name;
								case 1:
								case 2:
									return descending ? lhs.Time > rhs.Time : lhs.Time < rhs.Time;
							}

							return false;
						});
					}
				}

				for (const RenderPassMetric& metric : passMetrics) {
					if (filter[0] != '\0') {
						if (metric.Name.find(filter) == std::string::npos)
							continue;
					}

					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex(0);
					ImGui::TextUnformatted(metric.Name.c_str());

					ImGui::TableSetColumnIndex(1);

					const ImVec4 passColor = metric.Time < 1.0f ? Theme::GreenColor : metric.Time < 4.0f ? Theme::YellowColor : Theme::RedColor;
					ImGui::TextColored(passColor, "%.3f ms", metric.Time);

					ImGui::TableSetColumnIndex(2);

					const float frameShare = renderTime > 0.0f ? metric.Time / renderTime : 0.0f;

					ImGui::ProgressBar(
						std::clamp(frameShare, 0.0f, 1.0f),
						ImVec2(-1.0f, 0.0f),
						std::format("{:.1f}%", frameShare * 100.0f).c_str()
					);
				}

				ImGui::EndTable();
			}
		}

		if (ImGui::CollapsingHeader("Graphics Pipelines")) {
			const auto& allPipelineStatistics = Renderer::GetPipelineManager()->GetAllGraphicsPipelineStatistics();

			for (const auto& [pipelineName, pipelineStatistic] : allPipelineStatistics) {
				if (pipelineStatistic.IsEmpty())
					continue;

				if (!ImGui::TreeNodeEx(pipelineName.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth))
					continue;

				if (ImGui::BeginTable(std::format("##{}Statistics", pipelineName).c_str(), 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp)) {
					const auto DrawStatistic = [](const char* name, uint64_t value) {
						ImGui::TableNextRow();

						ImGui::TableSetColumnIndex(0);
						ImGui::TextDisabled("%s", name);

						ImGui::TableSetColumnIndex(1);
						ImGui::Text("%llu", value);
					};

					DrawStatistic("Input Assembly Vertices", pipelineStatistic.GetInputAssemblyVertexCount());
					DrawStatistic("Input Assembly Primitives", pipelineStatistic.GetInputAssemblyPrimitivesCount());
					DrawStatistic("Vertex Shader Invocations", pipelineStatistic.GetVertexShaderInvocations());
					DrawStatistic("Clipping Primitives Processed", pipelineStatistic.GetClippingStagePrimitivesProcessed());
					DrawStatistic("Clipping Primitives Output", pipelineStatistic.GetClippingStagePrimitivesOutput());
					DrawStatistic("Fragment Shader Invocations", pipelineStatistic.GetFragmentShaderInvocations());
					DrawStatistic("Tessellation Control Patches", pipelineStatistic.GetTesselationControlShaderPatches());
					DrawStatistic("Tessellation Evaluation Invocations", pipelineStatistic.GetTesselationEvaluationShaderInvocations());

					ImGui::EndTable();
				}

				ImGui::TreePop();
			}
		}

		if (ImGui::CollapsingHeader("Memory Viewer", ImGuiTreeNodeFlags_DefaultOpen)) {
			const float totalAllocated = static_cast<float>(appMetrics.GetTotalAllocated());
			const float totalFreed = static_cast<float>(appMetrics.GetTotalFreed());
			const float currentUsage = static_cast<float>(appMetrics.GetCurrentUsage());

			if (ImGui::BeginTable("MemoryStats", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp)) {
				const auto DrawMemoryValue = [](const char* name, float value) {
					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex(0);
					ImGui::TextDisabled("%s", name);

					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%.3f MB", value);
				};

				DrawMemoryValue("Total Allocated", totalAllocated);
				DrawMemoryValue("Total Freed", totalFreed);
				DrawMemoryValue("Current Usage", currentUsage);

				ImGui::EndTable();
			}

			ImGui::Spacing();

			const float currentMemoryRatio = totalAllocated > 0.0f ? currentUsage / totalAllocated : 0.0f;
			ImGui::TextDisabled("Live Allocation Ratio");
			ImGui::ProgressBar(std::clamp(currentMemoryRatio, 0.0f, 1.0f), ImVec2(-1.0f, 18.0f * uiScale), std::format("{:.2f} MB", currentUsage).c_str());
		}

		ImGui::End();

		ImGui::PopStyleVar(5);
		ImGui::PopStyleColor(13);
	}
}