#include "lypch.h"
#include "Window.h"

namespace Lucy {

	std::function<void(Event*)> Window::s_EventFunc;

	RefLucy<Window> Window::Create(const WindowSpecification& specs) {
#ifdef  LUCY_WINDOWS
		RefLucy<Window> window = CreateRef<WinWindow>();
		window->m_Specs = specs;
		return window;
#else
		LUCY_CRITICAL("Operating system not being supported!");
		LUCY_ASSERT(false);
#endif
	}

	void WinWindow::Init() {

		if (!glfwInit()) {
			LUCY_CRITICAL("GLFW init failed!");
			LUCY_ASSERT(false);
		}

		if (m_Specs.Architecture == RenderArchitecture::Vulkan) {
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		} else {
			glfwWindowHint(GLFW_DOUBLEBUFFER, m_Specs.DoubleBuffered);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		}
		glfwWindowHint(GLFW_RESIZABLE, m_Specs.Resizable);

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

		if (m_Specs.Architecture == RenderArchitecture::OpenGL) {
			glfwMakeContextCurrent(m_Window);
			glfwSwapInterval(m_Specs.VSync);
		}
	}

	void WinWindow::InitVulkanSurface(VkInstance instance) {
		LUCY_VULKAN_ASSERT(glfwCreateWindowSurface(instance, m_Window, nullptr, &m_Surface));
	}

	void WinWindow::DestroyVulkanSurface(VkInstance instance) {
		vkDestroySurfaceKHR(instance, m_Surface, nullptr);
	}

	void Window::SetEventCallback(std::function<void(Event*)> func) {
		s_EventFunc = func;
	}

	GLFWwindow* Window::Raw() {
		return m_Window;
	}

	void WinWindow::Update() {
		glfwGetWindowSize(m_Window, &m_Specs.Width, &m_Specs.Height);

		while (m_Specs.Width == 0 || m_Specs.Height == 0) {
			glfwGetWindowSize(m_Window, &m_Specs.Width, &m_Specs.Height);
			glfwWaitEvents();
		}
	}

	void WinWindow::PollEvents() {
		glfwPollEvents();

		//Adding events
		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int32_t width, int32_t height) {
			WindowResizeEvent evt{ width, height };
			s_EventFunc(&evt);
		});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window) {
			WindowCloseEvent evt{ window };
			s_EventFunc(&evt);
		});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int32_t key, int32_t scanCode, int32_t action, int32_t mods) {
			KeyEvent evt{ key, scanCode, action, mods };
			s_EventFunc(&evt);
		});

		glfwSetCharCallback(m_Window, [](GLFWwindow* window, uint32_t codePoint) {
			CharCallbackEvent evt{ codePoint };
			s_EventFunc(&evt);
		});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset) {
			ScrollEvent evt{ xOffset, yOffset };
			s_EventFunc(&evt);
		});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos) {
			CursorPosEvent evt{ xPos, yPos };
			s_EventFunc(&evt);
		});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int32_t button, int32_t action, int32_t mods) {
			MouseEvent evt{ button, action, mods };
			s_EventFunc(&evt);
		});
	}

	void WinWindow::Destroy() {
		glfwDestroyWindow(m_Window);
	}
}