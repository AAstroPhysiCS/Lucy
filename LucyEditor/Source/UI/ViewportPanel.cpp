#include "ViewportPanel.h"
#include "SceneExplorerPanel.h"

#include "Core/RenderPipeline.h"
#include "Renderer/Renderer.h"

#include "Events/EventHandler.h"
#include "Events/InputEvent.h"
#include "Events/WindowEvent.h"

#include "glm/gtc/type_ptr.hpp"

namespace Lucy {

	ViewportPanel& ViewportPanel::GetInstance() {
		static ViewportPanel s_Instance;
		return s_Instance;
	}

	void ViewportPanel::OnEvent(Event& e) {
		EventHandler::AddListener<KeyEvent>(e, [this](const KeyEvent& keyEvent) {
			if (keyEvent == KeyCode::V)
				UseSnap = !UseSnap;

			if (!m_IsViewportActive || !SceneExplorerPanel::GetInstance().GetEntityContext().IsValid()) return;

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
		LUCY_PROFILE_NEW_EVENT("ViewportPanel::Render");
		
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration;
		static bool pOpen = true;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
		ImGui::Begin("Viewport", &pOpen, flags);
		ImGui::PopStyleVar(2);

		m_IsViewportHovered = ImGui::IsWindowHovered();
		m_IsViewportActive = m_IsViewportHovered && ImGui::IsWindowFocused();
		m_IsOverAnyGizmo = IsOverAnyGizmoM();

		m_Size = ImGui::GetWindowSize();

		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan: {
				void* outputTextureID = m_RenderPipeline->GetOutputImage()->GetImGuiID();
				ImGui::Image((ImTextureID)outputTextureID, m_Size, {0, 1}, {1, 0});
				break;
			}
			default:
				LUCY_ASSERT(false);
		}

		auto [w, h] = m_RenderPipeline->GetViewportArea();
		if (w != m_Size.x || h != m_Size.y)
			EventHandler::DispatchImmediateEvent<ViewportAreaResizeEvent>((uint32_t)m_Size.x, (uint32_t)m_Size.y);

		const ImVec2& mousePos = ImGui::GetMousePos();
		const ImVec2& offset = ImGui::GetCursorPos();
		const ImVec2& windowPos = ImGui::GetWindowPos();

		m_ViewportMouseX = mousePos.x - windowPos.x - offset.x;
		m_ViewportMouseY = mousePos.y - windowPos.y;
		EventHandler::DispatchImmediateEvent<CursorPosEvent>(nullptr, m_ViewportMouseX, m_ViewportMouseY);

		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(windowPos.x, windowPos.y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());

		Entity& e = SceneExplorerPanel::GetInstance().GetEntityContext();
		if (e.IsValid()) {
			const auto& scene = SceneExplorerPanel::GetInstance().GetActiveScene();
			TransformComponent& t = e.GetComponent<TransformComponent>();
			float matrixTranslation[3], matrixRotation[3], matrixScale[3];
			t.CalculateMatrix();

			const auto& editorCamera = scene->GetEditorCamera();
			ImGuizmo::Manipulate(glm::value_ptr(editorCamera.GetViewMatrix()), glm::value_ptr(editorCamera.GetProjectionMatrix()),
								 CurrentGizmoOperation, ImGuizmo::MODE::LOCAL, glm::value_ptr(t.GetMatrix()), nullptr, UseSnap ? &SnapValue : nullptr, nullptr, nullptr);
			ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(t.GetMatrix()), matrixTranslation, matrixRotation, matrixScale);

			t.GetPosition().x = matrixTranslation[0]; t.GetPosition().y = matrixTranslation[1]; t.GetPosition().z = matrixTranslation[2];
			t.GetRotation().x = matrixRotation[0]; t.GetRotation().y = matrixRotation[1]; t.GetRotation().z = matrixRotation[2];
			t.GetScale().x = matrixScale[0]; t.GetScale().y = matrixScale[1]; t.GetScale().z = matrixScale[2];
		}

		ImGui::End();
	}

	bool ViewportPanel::IsViewportHovered() const {
		return m_IsViewportHovered;
	}

	bool ViewportPanel::IsViewportActive() const {
		return m_IsViewportActive;
	}

	bool ViewportPanel::IsOverAnyGizmo() const {
		return m_IsOverAnyGizmo;
	}

	bool ViewportPanel::IsOverTranslateGizmo() const {
		return ImGuizmo::IsOver(ImGuizmo::OPERATION::TRANSLATE) && CurrentGizmoOperation == ImGuizmo::OPERATION::TRANSLATE;
	}

	bool ViewportPanel::IsOverRotateGizmo() const {
		return ImGuizmo::IsOver(ImGuizmo::OPERATION::ROTATE) && CurrentGizmoOperation == ImGuizmo::OPERATION::ROTATE;
	}

	bool ViewportPanel::IsOverScaleGizmo() const {
		return ImGuizmo::IsOver(ImGuizmo::OPERATION::SCALE) && CurrentGizmoOperation == ImGuizmo::OPERATION::SCALE;
	}

	bool ViewportPanel::IsOverAnyGizmoM() const {
		return IsOverTranslateGizmo() || IsOverRotateGizmo() || IsOverScaleGizmo();
	}

	void ViewportPanel::SetRenderPipeline(Ref<RenderPipeline> renderPipeline) {
		m_RenderPipeline = renderPipeline;
	}
}