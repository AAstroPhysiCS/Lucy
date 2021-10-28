#include "ImGuiLayer.h"

#include "UI/SceneHierarchyPanel.h"
#include "UI/TaskbarPanel.h"
#include "UI/ViewportPanel.h"
#include "UI/PropertiesPanel.h"

#include "Events/InputEvent.h"
#include "Events/WindowEvent.h"
#include "Events/EventDispatcher.h"

#include "Renderer/Renderer.h"
#include "glad/glad.h"

#include <iostream>

namespace Lucy {

	ImGuiLayer::ImGuiLayer()
	{
		m_Panels.push_back(&SceneHierarchyPanel::GetInstance());
		m_Panels.push_back(&TaskbarPanel::GetInstance());
		m_Panels.push_back(&ViewportPanel::GetInstance());
		m_Panels.push_back(&PropertiesPanel::GetInstance());
	}

	void ImGuiLayer::Init(GLFWwindow* window)
	{
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.IniFilename = "lucyconfig.ini";

		io.Fonts->AddFontFromFileTTF("assets/fonts/ComicMono.ttf", 13);

		int32_t width, height;
		glfwGetWindowSize(window, &width, &height);
		io.DisplaySize = { (float)width, (float)height };

		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 460");
	}

	void ImGuiLayer::Begin()
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGuiIO& io = ImGui::GetIO();

		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

		static bool pOpen = true;

		ImGui::Begin("Main Panel", &pOpen, window_flags);
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
			ImGuiID dockSpaceId = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockSpaceId, { 0.0f, 0.0f }, ImGuiDockNodeFlags_PassthruCentralNode);
		}
		ImGui::PopStyleVar(3);
	}

	void ImGuiLayer::End()
	{
		ImGui::End(); //end of dockspace window

		ImGuiIO& io = ImGui::GetIO();
		float time = glfwGetTime();
		io.DeltaTime = time > 0.0f ? (time - m_Time) : (1.0f / 60.0f);
		m_Time = time;

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
	}

	void ImGuiLayer::OnRender()
	{
		for (Panel* panel : m_Panels) {
			panel->Render();
		}
	}

	void ImGuiLayer::OnEvent(Event& e)
	{
		EventDispatcher& dispatcher = EventDispatcher::GetInstance();
		dispatcher.Dispatch<ScrollEvent>(e, EventType::ScrollEvent, [&](ScrollEvent& e) {
			ImGuiIO& io = ImGui::GetIO();
			io.MouseWheelH += e.GetXOffset();
			io.MouseWheel += e.GetYOffset();
		});

		dispatcher.Dispatch<CursorPosEvent>(e, EventType::CursorPosEvent, [&](CursorPosEvent& e) {
			ImGuiIO& io = ImGui::GetIO();
			io.MousePos = { (float) e.GetXPos(), (float) e.GetYPos() };
		});

		dispatcher.Dispatch<KeyEvent>(e, EventType::KeyEvent, [&](KeyEvent& e) {
			ImGuiIO& io = ImGui::GetIO();
			int32_t action = e.GetAction();

			if (action == GLFW_PRESS) {
				io.KeysDown[e.GetKey()] = true;
			}
			else if (action == GLFW_RELEASE) {
				io.KeysDown[e.GetKey()] = false;
			}

			io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
			io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
			io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
			io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
		});

		dispatcher.Dispatch<CharCallbackEvent>(e, EventType::CharCallbackEvent, [&](CharCallbackEvent& e) {
			ImGuiIO& io = ImGui::GetIO();
			int32_t keyCode = e.GetCodePoint();
			if (keyCode > 0 && keyCode < 0x100000)
				io.AddInputCharacter((unsigned short) keyCode);
		});

		dispatcher.Dispatch<WindowResizeEvent>(e, EventType::WindowResizeEvent, [&](WindowResizeEvent& e) {
			ImGuiIO& io = ImGui::GetIO();
			io.DisplaySize = { (float) e.GetWidth(), (float) e.GetHeight() };
			io.DisplayFramebufferScale = { 1.0f, 1.0f };

			switch (Renderer::GetCurrentRenderContextType()) {
				case RenderContextType::OpenGL:
					glViewport(0, 0, e.GetWidth(), e.GetHeight());
					break;
				case RenderContextType::Vulkan:
					LUCY_CRITICAL("Vulkan not supported");
					LUCY_ASSERT(false);
					break;
			}
		});
	}

	void ImGuiLayer::Destroy()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
}