#pragma once

#include "Base.h"
#include <GLFW/glfw3.h>

#include "../Events/EventDispatcher.h"
#include "../Events/InputEvent.h"
#include "../Events/WindowEvent.h"

namespace Lucy {

	enum class WindowMode {
		FULLSCREEN, WINDOWED
	};

	struct WindowSpecification {
		uint32_t Width, Height;
		bool VSync, Resizable, DoubleBuffered;
		std::string Name;
		Lucy::WindowMode WindowMode = WindowMode::WINDOWED;
	};

	class Window
	{
	public:
		virtual ~Window() = default;

		virtual void PollEvents() = 0;
		virtual void Init() = 0;
		virtual void Destroy() = 0;

		GLFWwindow* Raw();

		static ScopeLucy<Window> Create(const WindowSpecification& specs);

	protected:
		WindowSpecification m_Specs;
		GLFWwindow* m_Window;
	};

	class WinWindow : public Window {

	private:
		void PollEvents();
		void Init();
		void Destroy();
	};
}