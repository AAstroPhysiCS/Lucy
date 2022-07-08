#pragma once

#define GLFW_INCLUDE_VULKAN

#include "Base.h"
#include "GLFW/glfw3.h"
#include "vulkan/vulkan.h"

#include "Events/Event.h"

namespace Lucy {

#define LUCY_BIND_FUNC(func) std::bind(func, this, std::placeholders::_1)

	enum class RenderArchitecture;

	enum class WindowMode {
		FULLSCREEN, WINDOWED
	};

	struct WindowCreateInfo {
		mutable int32_t Width = -1, Height = -1;
		bool VSync = false, Resizable = false;
		mutable std::string Name = "Window Specification failed!";
		Lucy::WindowMode WindowMode = WindowMode::WINDOWED;
	};

	class Window {
	public:
		virtual ~Window() = default;

		virtual void PollEvents() = 0;
		virtual void Init(RenderArchitecture architecture) = 0;
		virtual void InitVulkanSurface(VkInstance instance) = 0;
		virtual void DestroyVulkanSurface(VkInstance instance) = 0; //leaving to child class, to destroy it's contents.
		virtual void Destroy() = 0; //leaving to child class, to destroy it's contents.
		virtual void WaitEventsIfMinimized() = 0;

		void SetEventCallback(std::function<void(Event*)>);
		GLFWwindow* Raw();

		inline int32_t GetWidth() const { return m_CreateInfo.Width; }
		inline int32_t GetHeight() const { return m_CreateInfo.Height; }
		VkSurfaceKHR GetVulkanSurface() const { return m_Surface; }

		static Ref<Window> Create(const WindowCreateInfo& createInfo);
	protected:
		WindowCreateInfo m_CreateInfo;
		GLFWwindow* m_Window = nullptr;

		static std::function<void(Event*)> s_EventFunc;

		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
	};

	class WinWindow : public Window {
	public:
		virtual ~WinWindow() = default;
	private:
		void PollEvents() override;
		void Init(RenderArchitecture architecture) override;
		void InitVulkanSurface(VkInstance instance) override;
		void DestroyVulkanSurface(VkInstance instance) override;
		void Destroy() override;
		void WaitEventsIfMinimized() override;
	};
}