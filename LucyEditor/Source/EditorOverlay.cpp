#include "lypch.h"
#include "EditorOverlay.h"

#include "UI/SceneExplorerPanel.h"
#include "UI/TaskbarPanel.h"
#include "UI/ViewportPanel.h"
#include "UI/DetailsPanel.h"
#include "UI/DebugPanel.h"
#include "UI/RendererSettingsPanel.h"

#include "Events/EventHandler.h"
#include "Events/InputEvent.h"
#include "Events/WindowEvent.h"

#include "Renderer/Renderer.h"
#include "Renderer/RendererPasses.h"

namespace Lucy {

	EditorOverlay::EditorOverlay(const Ref<Scene>& scene) 
		: Overlay(scene) {
	}

	void EditorOverlay::OnRendererInit(const Ref<Window>& window) {
		m_Panels.push_back(&SceneExplorerPanel::GetInstance());
		m_Panels.push_back(&TaskbarPanel::GetInstance());
		m_Panels.push_back(&DetailsPanel::GetInstance());
		m_Panels.push_back(&DebugPanel::GetInstance());
		m_Panels.push_back(&ViewportPanel::GetInstance());
		m_Panels.push_back(&RendererSettingsPanel::GetInstance());

		auto& sceneExplorerPanel = SceneExplorerPanel::GetInstance();
		auto& viewportPanel = ViewportPanel::GetInstance();
		auto& metricsPanel = DebugPanel::GetInstance();
		metricsPanel.ToggleShow();

		sceneExplorerPanel.SetScene(m_Scene);

		viewportPanel.SetRenderPipeline(m_RenderPipeline);
	}

	void EditorOverlay::Begin() {
		LUCY_PROFILE_NEW_EVENT("EditorOverlay::Begin");

		auto currentArchitecture = Renderer::GetRenderArchitecture();
		if (currentArchitecture == RenderArchitecture::Vulkan)
			ImGui_ImplVulkan_NewFrame();

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

	void EditorOverlay::End() {
		LUCY_PROFILE_NEW_EVENT("EditorOverlay::End");

		ImGui::End(); //end of dockspace window

		ImGuiIO& io = ImGui::GetIO();
		double time = glfwGetTime();
		io.DeltaTime = (float) (time > 0.0f ? (time - m_Time) : (1.0f / 60.0f));
		m_Time = time;

		ImGui::Render();

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}

		Renderer::RenderImGui();
	}

	void EditorOverlay::Render() {
		LUCY_PROFILE_NEW_EVENT("EditorOverlay::Render");
		for (Panel* panel : m_Panels) {
			if (!panel->GetShow())
				continue;
			panel->Render();
		}
	}

	void EditorOverlay::OnEvent(Event& e) {
		EventHandler::AddListener<WindowResizeEvent>(e, [&](const WindowResizeEvent& e) {
			ImGuiIO& io = ImGui::GetIO();
			io.DisplaySize = { (float)e.GetWidth(), (float)e.GetHeight() };
			io.DisplayFramebufferScale = { 1.0f, 1.0f };
		});

		for (Panel* panel : m_Panels) {
			panel->OnEvent(e);
		}
	}

	void EditorOverlay::Destroy() {
		for (Panel* panel : m_Panels)
			panel->OnDestroy();

		auto currentArchitecture = Renderer::GetRenderArchitecture();
		if (currentArchitecture == RenderArchitecture::Vulkan)
			ImGui_ImplVulkan_Shutdown();

		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
}