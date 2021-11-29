#pragma once

#include "UI/Panel.h"
#include "imgui.h"
#include "ImGuizmo.h"

namespace Lucy {

	class ViewportPanel : public Panel
	{
	public:
		static ViewportPanel& GetInstance();

		inline ImGuizmo::OPERATION GetCurrentGizmoOperation() const { return CurrentGizmoOperation; }

		ImGuizmo::OPERATION CurrentGizmoOperation;
		float SnapValue = 1.0f;
		bool UseSnap = false;

		bool IsViewportHovered = false;
		bool IsViewportActive = false;
		bool IsOverAnyGizmo = false;

		void Render();
		virtual void OnEvent(Event& e) override;
	private:
		bool IsOverTranslateGizmo();
		bool IsOverRotateGizmo();
		bool IsOverScaleGizmo();
		bool IsOverAnyGizmoM();
	};
}

