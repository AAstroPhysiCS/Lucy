#include "ImGuiLayer.h"

#include "UI/SceneHierarchyPanel.h"
#include "UI/TaskbarPanel.h"
#include "UI/ViewportPanel.h"
#include "UI/PropertiesPanel.h"
#include "UI/PerformancePanel.h"

#include "Events/InputEvent.h"
#include "Events/WindowEvent.h"
#include "Events/EventDispatcher.h"
#include "Core/Input.h"

#include "Renderer/Renderer.h"
#include "Renderer/RenderPass.h"
#include "glad/glad.h"

#include <iostream>

namespace Lucy {

	ImGuiLayer::ImGuiLayer() {
		m_Panels.push_back(&SceneHierarchyPanel::GetInstance());
		m_Panels.push_back(&TaskbarPanel::GetInstance());
		m_Panels.push_back(&ViewportPanel::GetInstance());
		m_Panels.push_back(&PropertiesPanel::GetInstance());
		m_Panels.push_back(&PerformancePanel::GetInstance());
	}

	void ImGuiLayer::Init(RefLucy<Window>& window) {
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.IniFilename = "lucyconfig.ini";

		io.Fonts->AddFontFromFileTTF("assets/fonts/ComicMono.ttf", 13);

		int32_t width, height;
		glfwGetWindowSize(window->Raw(), &width, &height);
		io.DisplaySize = { (float)width, (float)height };

		ImGui_ImplGlfw_InitForOpenGL(window->Raw(), true);
		ImGui_ImplOpenGL3_Init("#version 460");
	}

	void ImGuiLayer::Begin(PerformanceMetrics& rendererMetrics) {
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGuizmo::SetOrthographic(false);
		ImGuizmo::BeginFrame();

		ImGuiIO& io = ImGui::GetIO();

		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

		static bool pOpen = true;

		ImGui::Begin("Main Panel", &pOpen, window_flags);
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
			ImGuiID dockSpaceId = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockSpaceId, { 0.0f, 0.0f }, ImGuiDockNodeFlags_PassthruCentralNode);
		}
		ImGui::PopStyleVar(3);
	}

	void ImGuiLayer::End() {

		ImGui::End(); //end of dockspace window

		ImGuiIO& io = ImGui::GetIO();
		float time = glfwGetTime();
		io.DeltaTime = time > 0.0f ? (time - m_Time) : (1.0f / 60.0f);
		m_Time = time;

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
	}

	void ImGuiLayer::OnRender() {
		for (Panel* panel : m_Panels) {
			panel->Render();
		}
	}

	void ImGuiLayer::OnEvent(Event& e) {
		EventDispatcher& dispatcher = EventDispatcher::GetInstance();
		dispatcher.Dispatch<ScrollEvent>(e, EventType::ScrollEvent, [&](ScrollEvent& e) {
			ImGuiIO& io = ImGui::GetIO();
			io.MouseWheelH += e.GetXOffset();
			io.MouseWheel += e.GetYOffset();
		});

		dispatcher.Dispatch<CursorPosEvent>(e, EventType::CursorPosEvent, [&](CursorPosEvent& e) {
			ImGuiIO& io = ImGui::GetIO();
			//io.MousePos = { (float)e.GetXPos(), (float)e.GetYPos() };
			Input::MouseX = io.MousePos.x;
			Input::MouseY = io.MousePos.y;
		});

		dispatcher.Dispatch<KeyEvent>(e, EventType::KeyEvent, [&](KeyEvent& e) {
			ImGuiIO& io = ImGui::GetIO();
			int32_t action = e.GetAction();

			if (action == GLFW_PRESS) {
				io.KeysDown[e.GetKey()] = true;
			} else if (action == GLFW_RELEASE) {
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
				io.AddInputCharacter((unsigned short)keyCode);
		});

		dispatcher.Dispatch<WindowResizeEvent>(e, EventType::WindowResizeEvent, [&](WindowResizeEvent& e) {
			ImGuiIO& io = ImGui::GetIO();
			io.DisplaySize = { (float)e.GetWidth(), (float)e.GetHeight() };
			io.DisplayFramebufferScale = { 1.0f, 1.0f };
		});

		for (Panel* panel : m_Panels) {
			panel->OnEvent(e);
		}
	}

	void ImGuiLayer::Destroy() {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
}