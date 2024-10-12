#pragma once

#include "Core/Panel.h"

#include "imgui.h"
#include "ImGuizmo.h"

namespace Lucy {

	class RenderPipeline;
	class GraphicsPipeline;

	class ViewportPanel : public Panel
	{
	public:
		static ViewportPanel& GetInstance();

		inline ImGuizmo::OPERATION GetCurrentGizmoOperation() const { return CurrentGizmoOperation; }

		ImGuizmo::OPERATION CurrentGizmoOperation = ImGuizmo::TRANSLATE;
		float SnapValue = 1.0f;
		bool UseSnap = false;

		bool IsViewportHovered() const;
		bool IsViewportActive() const;
		bool IsOverAnyGizmo() const;

		void SetRenderPipeline(Ref<RenderPipeline> renderPipeline);

		inline float GetViewportMouseX() const { return m_ViewportMouseX; }
		inline float GetViewportMouseY() const { return m_ViewportMouseY; }

		void OnEvent(Event& e) final override;
	private:
		void Render();

		ViewportPanel() = default;
		virtual ~ViewportPanel() = default;

		bool m_IsViewportHovered = false;
		bool m_IsViewportActive = false;
		bool m_IsOverAnyGizmo = false;

		bool IsOverTranslateGizmo() const;
		bool IsOverRotateGizmo() const;
		bool IsOverScaleGizmo() const;
		bool IsOverAnyGizmoM() const;

		ImVec2 m_Size;

		Ref<RenderPipeline> m_RenderPipeline = nullptr;

		float m_ViewportMouseX = 0, m_ViewportMouseY = 0;
	};
}

