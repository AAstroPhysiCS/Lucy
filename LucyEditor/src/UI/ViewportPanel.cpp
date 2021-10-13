#include "ViewportPanel.h"

#include "imgui.h"

namespace Lucy {
	
	ViewportPanel& ViewportPanel::GetInstance()
	{
		static ViewportPanel s_Instance;
		return s_Instance;
	}

	void ViewportPanel::Render()
	{
		ImGui::Begin("Viewport");

		//ImVec2& size = ImGui::GetWindowSize();
		//ImGui::Image(0, size, { 0, 1 }, { 1, 0 });

		ImGui::End();
	}

}