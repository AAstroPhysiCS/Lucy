#include "lypch.h"
#include "Window.h"

#include "Renderer/Context/RenderContext.h"
#include "../Events/InputEvent.h"
#include "../Events/WindowEvent.h"

#include "stb/stb_image.h"

namespace Lucy {

	std::function<void(Event*)> Window::s_EventFunc;

	Ref<Window> Window::Create(const WindowCreateInfo& createInfo) {
#ifdef  LUCY_WINDOWS
		Ref<Window> window = Memory::CreateRef<WinWindow>();
		window->m_CreateInfo = createInfo;
		return window;
#else
		LUCY_CRITICAL("Operating system not being supported!");
		LUCY_ASSERT(false);
#endif
	}

	void WinWindow::Init(RenderArchitecture architecture) {
		LUCY_ASSERT(m_CreateInfo.Width != 0 || m_CreateInfo.Height != 0, "Window size is 0.");
		LUCY_ASSERT(glfwInit(), "GLFW init failed!");

		if (architecture == RenderArchitecture::Vulkan) {
			LUCY_ASSERT(glfwVulkanSupported(), "Vulkan loader has not been found!");
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		} else {
			glfwWindowHint(GLFW_DOUBLEBUFFER, true);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		}
		glfwWindowHint(GLFW_RESIZABLE, m_CreateInfo.Resizable);

		switch (m_CreateInfo.WindowMode) {
			case WindowMode::FULLSCREEN: {
				GLFWmonitor* mainMonitor = glfwGetPrimaryMonitor();
				m_Window = glfwCreateWindow(m_CreateInfo.Width, m_CreateInfo.Height, m_CreateInfo.Title.c_str(), mainMonitor, nullptr);
				break;
			}
			case WindowMode::WINDOWED: {
				m_Window = glfwCreateWindow(m_CreateInfo.Width, m_CreateInfo.Height, m_CreateInfo.Title.c_str(), nullptr, nullptr);
				break;
			}
			default:
				LUCY_ASSERT(false);
				break;
		}

		//TODO: Icon for the window (later)
		//GLFWimage icon;
		//icon.pixels = stbi_load("Assets/Textures/lucy_logo.png", &icon.width, &icon.height, 0, STBI_rgb_alpha);
		//glfwSetWindowIcon(m_Window, 1, &icon);
	}

	void WinWindow::InitVulkanSurface(VkInstance instance) {
		LUCY_VK_ASSERT(glfwCreateWindowSurface(instance, m_Window, nullptr, &m_Surface));
	}

	void WinWindow::DestroyVulkanSurface(VkInstance instance) {
		vkDestroySurfaceKHR(instance, m_Surface, nullptr);
	}

	void Window::SetEventCallback(std::function<void(Event*)> func) {
		s_EventFunc = func;
	}

	void Window::SetTitle(const char* title) {
		m_CreateInfo.Title = title;
		glfwSetWindowTitle(m_Window, title);
	}

	GLFWwindow* Window::Raw() {
		return m_Window;
	}

	void WinWindow::WaitEventsIfMinimized() {
		glfwGetWindowSize(m_Window, &m_CreateInfo.Width, &m_CreateInfo.Height);

		while (m_CreateInfo.Width == 0 || m_CreateInfo.Height == 0) {
			glfwGetWindowSize(m_Window, &m_CreateInfo.Width, &m_CreateInfo.Height);
			glfwWaitEvents();
		}
	}

	void WinWindow::PollEvents() {
		glfwPollEvents();

		//Adding events
		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int32_t width, int32_t height) {
			WindowResizeEvent evt{ window, width, height };
			s_EventFunc(&evt);
		});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window) {
			WindowCloseEvent evt{ window };
			s_EventFunc(&evt);
		});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int32_t key, int32_t scanCode, int32_t action, int32_t mods) {
			KeyEvent evt{ window, key, scanCode, action, mods };
			s_EventFunc(&evt);
		});

		glfwSetCharCallback(m_Window, [](GLFWwindow* window, uint32_t codePoint) {
			CharCallbackEvent evt{ window, codePoint };
			s_EventFunc(&evt);
		});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset) {
			ScrollEvent evt{ window, xOffset, yOffset };
			s_EventFunc(&evt);
		});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos) {
			CursorPosEvent evt{ window, xPos, yPos };
			s_EventFunc(&evt);
		});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int32_t button, int32_t action, int32_t mods) {
			MouseEvent evt{ window, button, action, mods };
			s_EventFunc(&evt);
		});
	}

	void WinWindow::Destroy() {
		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}
}