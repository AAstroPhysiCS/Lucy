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

		ImGui::End();
	}

}