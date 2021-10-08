#pragma once

#include "Core/Layer.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "GLFW/glfw3.h"

namespace Lucy {
	class ImGuiLayer : public Layer
	{

	public:

		static ImGuiLayer* GetInstance() {
			if (!s_Instance) s_Instance = new ImGuiLayer();
			return s_Instance;
		}

		void Init(GLFWwindow* window);
		void Begin();
		void End();
		void OnRender();
		void OnEvent();
		void Destroy();

	private:
		ImGuiLayer() = default;

		uint32_t m_Time = 0;

		static ImGuiLayer* s_Instance;
	};
}