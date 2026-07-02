#pragma once

#define GLFW_INCLUDE_VULKAN

#include "Base.h"
#include "GLFW/glfw3.h"
#include "vulkan/vulkan.h"

#include "Events/Event.h"

namespace Lucy {

	enum class RenderArchitecture : uint8_t;

	enum class WindowMode {
		FULLSCREEN, WINDOWED
	};

	struct WindowCreateInfo {
		mutable int32_t Width = -1, Height = -1;
		bool VSync = false, Resizable = false;
		mutable std::string Title = "Window Specification failed!";
		Lucy::WindowMode WindowMode = WindowMode::WINDOWED;
	};

	struct WindowData {
		std::string Title;
		int32_t Width = 0;
		int32_t Height = 0;
		bool Minimized = false;

		std::function<void(std::unique_ptr<Event>)> EventCallback;
	};

	class Window {
	public:
		Window() = default;
		virtual ~Window() = default;

		virtual void PollEvents() = 0;
		virtual void Init(RenderArchitecture architecture) = 0;
		virtual void Destroy() = 0; //leaving to child class, to destroy it's contents.
		virtual void WaitEventsIfMinimized() = 0;
		virtual void SetEventCallback(const std::function<void(std::unique_ptr<Event>)>& eventCallbackFunc) = 0;

		GLFWwindow* Raw();

		inline int32_t GetWidth() const { return m_CreateInfo.Width; }
		inline int32_t GetHeight() const { return m_CreateInfo.Height; }
		
		void InitVulkanSurface(VkInstance instance);
		void DestroyVulkanSurface(VkInstance instance);
		inline VkSurfaceKHR GetVulkanSurface() const { return m_Surface; }

		void SetTitle(const char* title);
		inline const std::string& GetTitle() const { return m_CreateInfo.Title; }

		static Ref<Window> Create(const WindowCreateInfo& createInfo);
	protected:
		WindowCreateInfo m_CreateInfo;
		GLFWwindow* m_Window = nullptr;

		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

		WindowData m_Data;
	};

	class WinWindow : public Window {
	public:
		WinWindow() = default;
		virtual ~WinWindow() = default;
	private:
		void PollEvents() final override;
		
		void Init(RenderArchitecture architecture) final override;
		
		void Destroy() final override;

		void SetEventCallback(const std::function<void(std::unique_ptr<Event>)>& eventCallbackFunc) final override;
		void WaitEventsIfMinimized() final override;
	};
}