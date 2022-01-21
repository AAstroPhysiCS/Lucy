#pragma once

#include "Core/Base.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "ImGuizmo.h"

#include "GLFW/glfw3.h"

#include "Core/Layer.h"
#include "UI/Panel.h"

#include "Core/Window.h"

namespace Lucy {

	class ImGuiLayer : public Layer
	{
	public:
		static ImGuiLayer& GetInstance() {
			static ImGuiLayer s_Instance;
			return s_Instance;
		}

		void Init(RefLucy<Window> window);
		void Begin(PerformanceMetrics& performanceMetrics);
		void End();
		void OnRender();
		void OnEvent(Event& e);
		void Destroy();
	private:
		ImGuiLayer();

		uint32_t m_Time = 0;
		std::vector<Panel*> m_Panels;
	};
}