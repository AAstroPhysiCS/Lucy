#include "ViewportPanel.h"
#include "imgui.h"

#include "Renderer/Renderer.h"
#include "Renderer/Buffer/OpenGL/OpenGLFrameBuffer.h"

namespace Lucy {
	
	ViewportPanel& ViewportPanel::GetInstance()
	{
		static ViewportPanel s_Instance;
		return s_Instance;
	}

	void ViewportPanel::Render()
	{
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar;
		static bool pOpen = true;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
		ImGui::Begin("Viewport", &pOpen, flags);
		ImGui::PopStyleVar(2);

		auto& blittedFrameBuffer = std::static_pointer_cast<OpenGLFrameBuffer>(Renderer::GetMainFrameBuffer()->GetBlitted());
		auto& texture = blittedFrameBuffer->GetTexture(0);

		ImVec2& size = ImGui::GetWindowSize();
		ImGui::Image((ImTextureID) texture->GetID(), size, { 0, 1 }, { 1, 0 });

		ImGui::End();
	}
}