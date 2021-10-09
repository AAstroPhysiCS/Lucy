#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "GLFW/glfw3.h"

#include "Core/Layer.h"
#include "Events/Event.h"

namespace Lucy {
	class ImGuiLayer : public Layer
	{

	public:

		static ImGuiLayer& GetInstance() {
			static ImGuiLayer s_Instance;
			return s_Instance;
		}

		void Init(GLFWwindow* window);
		void Begin();
		void End();
		void OnRender();
		void OnEvent(Event& e);
		void Destroy();

	private:
		ImGuiLayer() = default;

		uint32_t m_Time = 0;
	};
}