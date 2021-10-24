#pragma once

#include "Core/Layer.h"

#include "Events/EventDispatcher.h"
#include "Events/InputEvent.h"

#include "GLFW/glfw3.h"

namespace Lucy {

	class EditorLayer : public Layer {
	public:
		static EditorLayer& GetInstance() {
			static EditorLayer s_Instance;
			return s_Instance;
		}

		void Begin();
		void End();
		void Init(GLFWwindow* window);
		void OnRender();
		void OnEvent(Event& e);
		void Destroy();
	private:
		EditorLayer() = default;

		GLFWwindow* m_Window = nullptr;
	};
}