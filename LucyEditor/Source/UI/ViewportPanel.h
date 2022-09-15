#pragma once

#include "Core/Panel.h"
#include "imgui.h"
#include "ImGuizmo.h"

namespace Lucy {

	class GraphicsPipeline;

	class ViewportPanel : public Panel
	{
	public:
		static ViewportPanel& GetInstance();

		inline ImGuizmo::OPERATION GetCurrentGizmoOperation() const { return CurrentGizmoOperation; }

		ImGuizmo::OPERATION CurrentGizmoOperation = ImGuizmo::TRANSLATE;
		float SnapValue = 1.0f;
		bool UseSnap = false;

		bool IsViewportHovered();
		bool IsViewportActive();
		bool IsOverAnyGizmo();

		void SetOnViewportResizeCallback(std::function<void()>&& callback);
		void SetViewportOutputPipeline(Ref<GraphicsPipeline> pipeline);

		void OnEvent(Event& e) final override;
	private:
		void Render();

		ViewportPanel() = default;
		virtual ~ViewportPanel() = default;

		bool m_IsViewportHovered = false;
		bool m_IsViewportActive = false;
		bool m_IsOverAnyGizmo = false;

		bool IsOverTranslateGizmo();
		bool IsOverRotateGizmo();
		bool IsOverScaleGizmo();
		bool IsOverAnyGizmoM();

		ImVec2 m_Size;

		Ref<GraphicsPipeline> m_ViewportOutputPipeline = nullptr;
		std::function<void()> m_RendererModuleOnViewportResize;
	};
}

