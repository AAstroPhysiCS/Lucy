#include "lypch.h"
#include "ImGuiOverlay.h"

#include "UI/SceneExplorerPanel.h"
#include "UI/TaskbarPanel.h"
#include "UI/ViewportPanel.h"
#include "UI/DetailsPanel.h"
#include "UI/ApplicationMetricsPanel.h"

#include "Events/InputEvent.h"
#include "Events/WindowEvent.h"
#include "Events/EventDispatcher.h"
#include "Core/Input.h"

#include "Renderer/Renderer.h"
#include "Renderer/VulkanRHI.h"
#include "Renderer/ViewportRenderer.h"
#include "glad/glad.h"

#include "Renderer/Context/VulkanContext.h"

namespace Lucy {

	ImGuiOverlay::ImGuiOverlay() {
		m_Panels.push_back(&SceneExplorerPanel::GetInstance());
		m_Panels.push_back(&TaskbarPanel::GetInstance());
		m_Panels.push_back(&DetailsPanel::GetInstance());
		m_Panels.push_back(&ApplicationMetricsPanel::GetInstance());
		m_Panels.push_back(&ViewportPanel::GetInstance());
	}

	void ImGuiOverlay::Init(Ref<Window> window, Scene& scene) {
		m_Scene = &scene;

		SceneExplorerPanel::GetInstance().SetScene(m_Scene);

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

		auto currentArchitecture = Renderer::GetCurrentRenderArchitecture();
		if (currentArchitecture == RenderArchitecture::OpenGL) {
			ImGui_ImplGlfw_InitForOpenGL(window->Raw(), true);
			ImGui_ImplOpenGL3_Init("#version 460");
		} else if (currentArchitecture == RenderArchitecture::Vulkan) {
			ImGui_ImplGlfw_InitForVulkan(window->Raw(), true);
			Renderer::Enqueue([&]() mutable {
				auto& vulkanContext = Renderer::GetCurrentRenderer()->m_RenderContext.As<VulkanContext>();
				VulkanDevice device = VulkanDevice::Get();

				m_ImGuiPool = Memory::CreateRef<VulkanDescriptorPool>(m_PoolSpecs);

				ImGui_ImplVulkan_InitInfo initInfo{};
				initInfo.Instance = vulkanContext->GetVulkanInstance();
				initInfo.PhysicalDevice = device.GetPhysicalDevice();
				initInfo.Device = device.GetLogicalDevice();
				initInfo.Queue = device.GetGraphicsQueue();
				initInfo.DescriptorPool = m_ImGuiPool->GetVulkanHandle();
				initInfo.MinImageCount = 3;
				initInfo.ImageCount = 3;
				initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
				initInfo.CheckVkResultFn = VulkanMessageCallback::ImGui_DebugCallback;

				ImGui_ImplVulkan_Init(&initInfo, ViewportRenderer::GetImGuiPipeline().UIRenderPass.As<VulkanRenderPass>()->GetVulkanHandle());
				VulkanRHI::RecordSingleTimeCommand(ImGui_ImplVulkan_CreateFontsTexture);
				ImGui_ImplVulkan_DestroyFontUploadObjects();
			});
		}
	}

	void ImGuiOverlay::Begin() {
		auto currentArchitecture = Renderer::GetCurrentRenderArchitecture();
		if (currentArchitecture == RenderArchitecture::OpenGL) {
			ImGui_ImplOpenGL3_NewFrame();
		} else if (currentArchitecture == RenderArchitecture::Vulkan) {
			ImGui_ImplVulkan_NewFrame();
		}

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

	void ImGuiOverlay::End() {
		ImGui::End(); //end of dockspace window

		ImGuiIO& io = ImGui::GetIO();
		float time = glfwGetTime();
		io.DeltaTime = time > 0.0f ? (time - m_Time) : (1.0f / 60.0f);
		m_Time = time;

		ImGui::Render();
		SendImGuiDataToGPU();

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
	}

	void ImGuiOverlay::SendImGuiDataToGPU() {
		auto currentArchitecture = Renderer::GetCurrentRenderArchitecture();

		if (currentArchitecture == RenderArchitecture::OpenGL) {
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		} else if (currentArchitecture == RenderArchitecture::Vulkan) {
			Renderer::SetUIDrawData([](VkCommandBuffer commandBuffer) {
				ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
			});
		}
	}

	void ImGuiOverlay::Render() {
		Begin();
		for (Panel* panel : m_Panels) {
			panel->Render();
		}
		End();
	}

	void ImGuiOverlay::OnEvent(Event& e) {
		EventDispatcher& dispatcher = EventDispatcher::GetInstance();
		dispatcher.Dispatch<ScrollEvent>(e, EventType::ScrollEvent, [&](ScrollEvent& e) {
			ImGuiIO& io = ImGui::GetIO();
			io.AddMouseWheelEvent(e.GetXOffset(), e.GetYOffset());
		});

		dispatcher.Dispatch<CursorPosEvent>(e, EventType::CursorPosEvent, [&](CursorPosEvent& e) {
			ImGuiIO& io = ImGui::GetIO();
			Input::MouseX = e.GetXPos();
			Input::MouseY = e.GetYPos();
			ImGui_ImplGlfw_CursorPosCallback(e.GetWindowHandle(), Input::MouseX, Input::MouseY);
		});
		
		dispatcher.Dispatch<MouseEvent>(e, EventType::MouseEvent, [&](MouseEvent& e) {
			ImGui_ImplGlfw_MouseButtonCallback(e.GetWindowHandle(), e.GetButton(), e.GetAction(), e.GetMods());
		});

		dispatcher.Dispatch<KeyEvent>(e, EventType::KeyEvent, [&](KeyEvent& e) {
			ImGui_ImplGlfw_KeyCallback(e.GetWindowHandle(), e.GetKey(), e.GetScanCode(), e.GetAction(), e.GetMods());
		});

		dispatcher.Dispatch<CharCallbackEvent>(e, EventType::CharCallbackEvent, [&](CharCallbackEvent& e) {
			ImGui_ImplGlfw_CharCallback(e.GetWindowHandle(), e.GetCodePoint());
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

	void ImGuiOverlay::Destroy() {
		auto currentArchitecture = Renderer::GetCurrentRenderArchitecture();
		if (currentArchitecture == RenderArchitecture::OpenGL) {
			ImGui_ImplOpenGL3_Shutdown();
		} else if (currentArchitecture == RenderArchitecture::Vulkan) {
			ImGui_ImplVulkan_Shutdown();
			m_ImGuiPool->Destroy();
		}
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
}