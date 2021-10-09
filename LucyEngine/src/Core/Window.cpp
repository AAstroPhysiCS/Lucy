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
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);

		switch (m_Specs.WindowMode) {
			case WindowMode::FULLSCREEN: {
				GLFWmonitor* mainMonitor = glfwGetPrimaryMonitor();
				m_Window = glfwCreateWindow(m_Specs.Width, m_Specs.Height, m_Specs.Name.c_str(), mainMonitor, nullptr);
				break;
			}
			case WindowMode::WINDOWED: {
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

	GLFWwindow* Window::Raw()
	{
		return m_Window;
	}

	void WinWindow::PollEvents()
	{
		//Clearing
		EventDispatcher* evtDispatcher = EventDispatcher::GetInstance();
		evtDispatcher->GetEventPool().clear();

		glfwPollEvents();

		//Adding events
		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height) {
			EventDispatcher* evtDispatcher = EventDispatcher::GetInstance();
			WindowResizeEvent evt{ width, height };
			evtDispatcher->PushEvent<WindowResizeEvent>(evt);
		});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window) {
			EventDispatcher* evtDispatcher = EventDispatcher::GetInstance();
			WindowCloseEvent evt{ window };
			evtDispatcher->PushEvent<WindowCloseEvent>(evt);
		});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scanCode, int action, int mods) {
			EventDispatcher* evtDispatcher = EventDispatcher::GetInstance();
			KeyEvent evt{ key, scanCode, action, mods };
			evtDispatcher->PushEvent<KeyEvent>(evt);
		});

		glfwSetCharCallback(m_Window, [](GLFWwindow* window, uint32_t codePoint) {
			EventDispatcher* evtDispatcher = EventDispatcher::GetInstance();
			CharCallbackEvent evt{ codePoint };
			evtDispatcher->PushEvent<CharCallbackEvent>(evt);
		});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset) {
			EventDispatcher* evtDispatcher = EventDispatcher::GetInstance();
			ScrollEvent evt{ xOffset, yOffset };
			evtDispatcher->PushEvent<ScrollEvent>(evt);
		});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos) {
			EventDispatcher* evtDispatcher = EventDispatcher::GetInstance();
			CursorPosEvent evt{ xPos, yPos };
			evtDispatcher->PushEvent<CursorPosEvent>(evt);
		});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods) {
			EventDispatcher* evtDispatcher = EventDispatcher::GetInstance();
			MouseEvent evt{ button, action, mods };
			evtDispatcher->PushEvent<MouseEvent>(evt);
		});
	}

	void WinWindow::Destroy()
	{
		glfwDestroyWindow(m_Window);
	}

}