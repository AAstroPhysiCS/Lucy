#pragma once

#include "Base.h"
#include <GLFW/glfw3.h>

namespace Lucy {

	enum WindowMode {
		FULLSCREEN, WINDOWED
	};

	struct WindowSpecification {
		uint32_t Width, Height;
		bool VSync, Resizable, DoubleBuffered;
		std::string Name;
		WindowMode WindowMode = WindowMode::WINDOWED;
	};

	class Window
	{
	public:
		virtual ~Window() = default;

		virtual void Update() = 0;
		virtual void Init() = 0;

		GLFWwindow* GetRaw() {
			return m_Window;
		}

		static ScopeLucy<Window> Create(const WindowSpecification& specs);

	protected:
		WindowSpecification m_Specs;
		GLFWwindow* m_Window;

	};

	class WinWindow : public Window {

	private:
		void Update() override;
		void Init() override;

	};

}

