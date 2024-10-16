#include "TaskbarPanel.h"
#include "DebugPanel.h"

#include "imgui.h"

namespace Lucy {

	TaskbarPanel& TaskbarPanel::GetInstance() {
		static TaskbarPanel s_Instance;
		return s_Instance;
	}

	void TaskbarPanel::Render() {
		LUCY_PROFILE_NEW_EVENT("TaskbarPanel::Render");

		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("New Scene")) {
					//new scene
				}

				if (ImGui::MenuItem("Open a scene", "Ctrl+O")) {
					//open a scene
				}

				if (ImGui::MenuItem("Save the scene", "Ctrl+S")) {
					//save the scene
				}

				if (ImGui::MenuItem("Save as..")) {
					//save as
				}
				ImGui::EndMenu();
			}

			ImGui::Spacing();

			if (ImGui::BeginMenu("Edit")) {
				if (ImGui::MenuItem("Back", "Ctrl+Z")) {
				}

				if (ImGui::MenuItem("Forward", "Ctrl+Y")) {
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Panel")) {
				if (bool show = DebugPanel::GetInstance().GetShow(); ImGui::MenuItem("Debug", 0, show))
					DebugPanel::GetInstance().ToggleShow();
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
		ImGui::Spacing();
	}
}