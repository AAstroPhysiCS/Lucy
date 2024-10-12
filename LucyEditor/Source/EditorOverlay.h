#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "ImGuizmo.h"

#include "Core/Overlay.h"
#include "Core/Panel.h"

namespace Lucy {

	class Scene;
	class Window;

	class EditorOverlay : public Overlay {
	public:
		EditorOverlay(const Ref<Scene>& scene);
		virtual ~EditorOverlay() = default;

		void Begin() final override;
		void Render() final override;
		void End() final override;

		void OnRendererInit(const Ref<Window>& window) final override;

		void OnEvent(Event& e) final override;
		void Destroy() final override;
	private:
		double m_Time = 0;
		std::vector<Panel*> m_Panels;
	};
}