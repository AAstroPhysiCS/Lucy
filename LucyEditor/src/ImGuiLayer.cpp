#include "ImGuiLayer.h"

namespace Lucy {

	ImGuiLayer* ImGuiLayer::s_Instance = nullptr;


	void ImGuiLayer::Init(GLFWwindow* window)
	{
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;

		int32_t width, height;
		glfwGetWindowSize(window, &width, &height);
		io.DisplaySize = { (float) width, (float) height };

		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 450");
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
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
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
		//here comes the panel render func
		ImGui::ShowDemoWindow();
	}

	void ImGuiLayer::OnEvent()
	{
	}

	void ImGuiLayer::Destroy()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

}