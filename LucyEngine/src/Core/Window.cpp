#include "Window.h"

namespace Lucy {

	ScopeLucy<Window> Window::Create(const WindowSpecification& specs)
	{
#ifdef  LUCY_WINDOWS
		ScopeLucy<Window> window = CreateScope<WinWindow>();
		window->m_Specs = specs;
		return window;
#else
		LUCY_CRITICAL("Operating system not being supported!")
			LUCY_ASSERT(false);
#endif
	}

	void WinWindow::Init() {

		if (!glfwInit()) {
			LUCY_CRITICAL("GLFW init failed!");
			LUCY_ASSERT(false);
		}

		glfwWindowHint(GLFW_RESIZABLE, m_Specs.Resizable);
		glfwWindowHint(GLFW_DOUBLEBUFFER, m_Specs.DoubleBuffered);

		switch (m_Specs.WindowMode) {
			case FULLSCREEN: {
				GLFWmonitor* mainMonitor = glfwGetPrimaryMonitor();
				m_Window = glfwCreateWindow(m_Specs.Width, m_Specs.Height, m_Specs.Name.c_str(), mainMonitor, nullptr);
				break;
			}
			case WINDOWED: {
				m_Window = glfwCreateWindow(m_Specs.Width, m_Specs.Height, m_Specs.Name.c_str(), nullptr, nullptr);
				break;
			}
			default:
				LUCY_CRITICAL("Should not happen?");
				LUCY_ASSERT(false);
				break;
		}

		glfwMakeContextCurrent(m_Window);
		glfwSwapInterval(m_Specs.VSync);
	}

	void WinWindow::Update()
	{
		glfwPollEvents();
	}

}
