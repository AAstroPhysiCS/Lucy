#pragma once

#include "Base.h"
#include "GLFW/glfw3.h"

#include "../Renderer/Buffer/Buffer.h"

#include "../Events/EventDispatcher.h"
#include "../Events/InputEvent.h"
#include "../Events/WindowEvent.h"

namespace Lucy {

	enum class WindowMode {
		FULLSCREEN, WINDOWED
	};

	struct WindowSpecification {
		int32_t Width = -1, Height = -1;
		bool VSync = false, Resizable = false, DoubleBuffered = false;
		std::string Name = "Window Specification failed!";
		Lucy::WindowMode WindowMode = WindowMode::WINDOWED;
	};

	class Window {
	public:
		virtual ~Window() = default;

		virtual void PollEvents() = 0;
		virtual void Init() = 0;
		virtual void Destroy() = 0;
		virtual void Update() = 0;

		void SetEventCallback(std::function<void(Event*)>);
		GLFWwindow* Raw();

		inline int32_t GetWidth() const { return m_Specs.Width; }
		inline int32_t GetHeight() const { return m_Specs.Height; }

		static RefLucy<Window> Create(const WindowSpecification& specs);
	protected:
		WindowSpecification m_Specs;
		GLFWwindow* m_Window;

		static std::function<void(Event*)> s_EventFunc;
	};

	class WinWindow : public Window {
	private:
		void PollEvents();
		void Init();
		void Destroy();
		void Update();
	};
}