#include "ContentBrowserPanel.h"

#include "Core/Application.h"

#include "imgui.h"

namespace Lucy {
	
	ContentBrowserPanel& ContentBrowserPanel::GetInstance() {
		static ContentBrowserPanel s_Instance;
		return s_Instance;
	}

	void ContentBrowserPanel::Render() {
		ImGui::Begin("Content Browser");

		//const auto& workingDirectory = Application::Get()->GetFilesystem().GetCurrentWorkingDirectory();

		ImGui::End();
	}
}
