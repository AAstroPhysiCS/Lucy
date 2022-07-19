#include "ViewportPanel.h"
#include "SceneExplorerPanel.h"

#include "Renderer/Renderer.h"
#include "Renderer/RenderPass.h"
#include "Renderer/ViewportRenderer.h"

#include "Renderer/OpenGLRHI.h"
#include "Renderer/VulkanRHI.h"
#include "Renderer/Context/VulkanSwapChain.h"

#include "Renderer/Memory/Buffer/OpenGL/OpenGLFrameBuffer.h"
#include "Renderer/Memory/Buffer/Vulkan/VulkanFrameBuffer.h"

#include "imgui_impl_vulkan.h"

namespace Lucy {

	ViewportPanel& ViewportPanel::GetInstance() {
		static ViewportPanel s_Instance;
		return s_Instance;
	}

	void ViewportPanel::OnEvent(Event& e) {
		EventDispatcher& dispatcher = EventDispatcher::GetInstance();
		dispatcher.Dispatch<KeyEvent>(e, EventType::KeyEvent, [this](const KeyEvent& keyEvent) {
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

		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL: {
				auto& blittedFrameBuffer = ViewportRenderer::GetGeometryPipeline()->GetFrameBuffer().As<OpenGLFrameBuffer>()->GetBlitted();
				void* outputTextureID = (void*)blittedFrameBuffer->GetTexture(0)->GetID();
				ImGui::Image(outputTextureID, m_Size, { 0, 1 }, { 1, 0 });
				break;
			}
			case RenderArchitecture::Vulkan: {
				VulkanSwapChain& swapChain = VulkanSwapChain::Get();
				void* outputTextureID = ViewportRenderer::GetGeometryPipeline()->GetFrameBuffer().As<VulkanFrameBuffer>()->GetImages()[swapChain.GetCurrentFrameIndex()]->GetImGuiID();
				ImGui::Image(outputTextureID, m_Size);
				break;
			}
			default:
				LUCY_ASSERT(false);
		}

		auto [w, h] = Renderer::GetViewportSize();
		if (w != m_Size.x || h != m_Size.y) {
			Renderer::SetViewportSize(m_Size.x, m_Size.y);
			Renderer::OnViewportResize();
		}

		const ImVec2& mousePos = ImGui::GetMousePos();
		const ImVec2& offset = ImGui::GetCursorPos();
		const ImVec2& windowPos = ImGui::GetWindowPos();

		Renderer::SetViewportMouse(mousePos.x - windowPos.x - offset.x,
								   mousePos.y - windowPos.y);
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(windowPos.x, windowPos.y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());

		Entity& e = SceneExplorerPanel::GetInstance().GetEntityContext();
		if (e.IsValid()) {
			EditorCamera& editorCamera = SceneExplorerPanel::GetInstance().GetActiveScene()->GetEditorCamera();
			TransformComponent& t = e.GetComponent<TransformComponent>();
			float matrixTranslation[3], matrixRotation[3], matrixScale[3];
			t.CalculateMatrix();
			ImGuizmo::Manipulate(glm::value_ptr(editorCamera.GetViewMatrix()), glm::value_ptr(editorCamera.GetProjectionMatrix()),
								 CurrentGizmoOperation, ImGuizmo::MODE::LOCAL, glm::value_ptr(t.GetMatrix()), nullptr, UseSnap ? &SnapValue : nullptr, nullptr, nullptr);
			ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(t.GetMatrix()), matrixTranslation, matrixRotation, matrixScale);
			t.GetPosition().x = matrixTranslation[0]; t.GetPosition().y = matrixTranslation[1]; t.GetPosition().z = matrixTranslation[2];
			t.GetRotation().x = matrixRotation[0]; t.GetRotation().y = matrixRotation[1]; t.GetRotation().z = matrixRotation[2];
			t.GetScale().x = matrixScale[0]; t.GetScale().y = matrixScale[1]; t.GetScale().z = matrixScale[2];
		}

		ImGui::End();
	}

	bool ViewportPanel::IsViewportHovered() {
		return m_IsViewportHovered;
	}

	bool ViewportPanel::IsViewportActive() {
		return m_IsViewportActive;
	}

	bool ViewportPanel::IsOverAnyGizmo() {
		return m_IsOverAnyGizmo;
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