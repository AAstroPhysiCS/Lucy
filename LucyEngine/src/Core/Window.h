#pragma once

#define GLFW_INCLUDE_VULKAN

#include "Base.h"
#include "GLFW/glfw3.h"

#include "../Renderer/Buffer/Buffer.h"

#include "../Events/EventDispatcher.h"
#include "../Events/InputEvent.h"
#include "../Events/WindowEvent.h"

#include "Renderer/Context/RenderContext.h"

#include "vulkan/vulkan.h"

namespace Lucy {

	enum class WindowMode {
		FULLSCREEN, WINDOWED
	};

	struct WindowSpecification {
		int32_t Width = -1, Height = -1;
		bool VSync = false, Resizable = false, DoubleBuffered = false;
		std::string Name = "Window Specification failed!";
		Lucy::WindowMode WindowMode = WindowMode::WINDOWED;
		RenderArchitecture Architecture;
	};

	class Window {
	public:
		virtual ~Window() = default;

		virtual void PollEvents() = 0;
		virtual void Init() = 0;
		virtual void InitVulkanSurface(VkInstance instance) = 0;
		virtual void DestroyVulkanSurface(VkInstance instance) = 0;
		virtual void Destroy() = 0;
		virtual void Update() = 0;

		void SetEventCallback(std::function<void(Event*)>);
		GLFWwindow* Raw();

		inline int32_t GetWidth() const { return m_Specs.Width; }
		inline int32_t GetHeight() const { return m_Specs.Height; }

		static RefLucy<Window> Create(const WindowSpecification& specs);
	protected:
		WindowSpecification m_Specs;
		GLFWwindow* m_Window = nullptr;

		static std::function<void(Event*)> s_EventFunc;

		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

		friend class VulkanDevice;
		friend class VulkanSwapChain;
	};

	class WinWindow : public Window {
	private:
		void PollEvents() override;
		void Init() override;
		void InitVulkanSurface(VkInstance instance) override;
		void DestroyVulkanSurface(VkInstance instance) override;
		void Destroy() override;
		void Update() override;
	};
}