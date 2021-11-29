#include "ViewportPanel.h"
#include "SceneHierarchyPanel.h"

#include "Renderer/Renderer.h"
#include "Renderer/RenderPass.h"
#include "Renderer/Buffer/OpenGL/OpenGLFrameBuffer.h"

namespace Lucy {

	ViewportPanel& ViewportPanel::GetInstance() {
		static ViewportPanel s_Instance;
		return s_Instance;
	}

	void ViewportPanel::OnEvent(Event& e) {
		EventDispatcher& dispatcher = EventDispatcher::GetInstance();
		dispatcher.Dispatch<KeyEvent>(e, EventType::KeyEvent, [this](KeyEvent& keyEvent) {
			if (keyEvent == KeyCode::V)
				UseSnap = !UseSnap;

			if (!IsViewportActive || !SceneHierarchyPanel::GetInstance().GetEntityContext().IsValid()) return;

			if (keyEvent == KeyCode::T) {
				CurrentGizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
				SnapValue = 0.5f;
			}
			if (keyEvent == KeyCode::R) {
				CurrentGizmoOperation = ImGuizmo::OPERATION::ROTATE;
				SnapValue = 10.0f;
			}
			if (keyEvent == KeyCode::E) {
				CurrentGizmoOperation = ImGuizmo::OPERATION::SCALE;
				SnapValue = 0.5f;
			}
		});
	}

	void ViewportPanel::Render() {
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus;
		static bool pOpen = true;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
		ImGui::Begin("Viewport", &pOpen, flags);
		ImGui::PopStyleVar(2);

		IsViewportHovered = ImGui::IsWindowHovered();
		IsViewportActive = IsViewportHovered && ImGui::IsWindowFocused();
		IsOverAnyGizmo = IsOverAnyGizmoM();

		auto& blittedFrameBuffer = As(Renderer::GetGeometryPass()->GetFrameBuffer()->GetBlitted(), OpenGLFrameBuffer);
		auto& texture = blittedFrameBuffer->GetTexture(0);

		ImVec2& size = ImGui::GetWindowSize();
		ImGui::Image((ImTextureID)texture->GetID(), size, { 0, 1 }, { 1, 0 });

		auto [w, h] = Renderer::GetViewportSize();
		if (w != size.x || h != size.y)
			Renderer::OnFramebufferResize(size.x, size.y);

		ImVec2& mousePos = ImGui::GetMousePos();
		ImVec2& offset = ImGui::GetCursorPos();
		ImVec2& windowPos = ImGui::GetWindowPos();

		Renderer::SetViewportMousePosition(mousePos.x - windowPos.x - offset.x,
										   mousePos.y - windowPos.y);
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());

		Entity& e = SceneHierarchyPanel::GetInstance().GetEntityContext();
		if (e.IsValid()) {
			Camera* editorCamera = Renderer::GetActiveCamera();
			TransformComponent& t = e.GetComponent<TransformComponent>();
			float matrixTranslation[3], matrixRotation[3], matrixScale[3];
			t.CalculateMatrix();
			ImGuizmo::Manipulate(glm::value_ptr(editorCamera->GetViewMatrix()), glm::value_ptr(editorCamera->GetProjectionMatrix()),
								 CurrentGizmoOperation, ImGuizmo::MODE::LOCAL, glm::value_ptr(t.GetMatrix()), nullptr, UseSnap ? &SnapValue : nullptr, nullptr, nullptr);
			ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(t.GetMatrix()), matrixTranslation, matrixRotation, matrixScale);
			t.GetPosition().x = matrixTranslation[0]; t.GetPosition().y = matrixTranslation[1]; t.GetPosition().z = matrixTranslation[2];
			t.GetRotation().x = matrixRotation[0]; t.GetRotation().y = matrixRotation[1]; t.GetRotation().z = matrixRotation[2];
			t.GetScale().x = matrixScale[0]; t.GetScale().y = matrixScale[1]; t.GetScale().z = matrixScale[2];
		}

		ImGui::End();
	}
	
	bool ViewportPanel::IsOverTranslateGizmo() {
		return ImGuizmo::IsOver(ImGuizmo::OPERATION::TRANSLATE) && CurrentGizmoOperation == ImGuizmo::OPERATION::TRANSLATE;
	}

	bool ViewportPanel::IsOverRotateGizmo() {
		return ImGuizmo::IsOver(ImGuizmo::OPERATION::ROTATE) && CurrentGizmoOperation == ImGuizmo::OPERATION::ROTATE;
	}
	
	bool ViewportPanel::IsOverScaleGizmo() {
		return ImGuizmo::IsOver(ImGuizmo::OPERATION::SCALE) && CurrentGizmoOperation == ImGuizmo::OPERATION::SCALE;
	}
	
	bool ViewportPanel::IsOverAnyGizmoM() {
		return IsOverTranslateGizmo() || IsOverRotateGizmo() || IsOverScaleGizmo();
	}
}