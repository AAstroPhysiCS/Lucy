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
		uint32_t Width, Height;
		bool VSync, Resizable, DoubleBuffered;
		std::string Name;
		Lucy::WindowMode WindowMode = WindowMode::WINDOWED;
	};

	class Window {
	public:
		virtual ~Window() = default;

		virtual void PollEvents() = 0;
		virtual void Init() = 0;
		virtual void Destroy() = 0;

		void SetEventCallback(std::function<void(Event*)>);
		GLFWwindow* Raw();

		inline uint32_t GetWidth() const { return m_Specs.Width; }
		inline uint32_t GetHeight() const { return m_Specs.Height; }

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
	};
}