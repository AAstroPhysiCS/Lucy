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
#include "Renderer/VulkanRenderer.h"
#include "Renderer/VulkanRenderPass.h"
#include "Renderer/Buffer/Vulkan/VulkanFrameBuffer.h"
#include "glad/glad.h"

#include "Renderer/Context/VulkanContext.h"

#include <iostream>

namespace Lucy {

	ImGuiLayer::ImGuiLayer() {
		m_Panels.push_back(&SceneHierarchyPanel::GetInstance());
		m_Panels.push_back(&TaskbarPanel::GetInstance());
		//m_Panels.push_back(&ViewportPanel::GetInstance());
		m_Panels.push_back(&PropertiesPanel::GetInstance());
		m_Panels.push_back(&PerformancePanel::GetInstance());
	}

	void ImGuiLayer::Init(RefLucy<Window> window) {
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
			auto& renderContext = Renderer::GetCurrentRenderer()->m_RenderContext;

			ImGui::CreateContext();
			ImGui_ImplGlfw_InitForVulkan(window->Raw(), true);

			auto& vulkanContext = As(renderContext, VulkanContext);
			VulkanDevice device = VulkanDevice::Get();
			RenderPassSpecification renderPassSpecs;
			renderPassSpecs.AttachmentReferences.push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
			renderPassSpecs.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
			renderPassSpecs.Descriptor.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			renderPassSpecs.Descriptor.StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
			renderPassSpecs.Descriptor.StencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			renderPassSpecs.Descriptor.StencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
			renderPassSpecs.Descriptor.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			renderPassSpecs.Descriptor.FinalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			m_RenderPass = As(RenderPass::Create(renderPassSpecs), VulkanRenderPass);
			Renderer::Dispatch(); //for renderpass only

			m_ImGuiPool = CreateRef<VulkanDescriptorPool>(m_PoolSpecs);

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
			ImGui_ImplVulkan_Init(&initInfo, m_RenderPass->GetVulkanHandle());

			VulkanRenderer::RecordSingleTimeCommand(ImGui_ImplVulkan_CreateFontsTexture);

			ImGui_ImplVulkan_DestroyFontUploadObjects();
		}
	}

	void ImGuiLayer::Begin(PerformanceMetrics& rendererMetrics) {
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

	void ImGuiLayer::End() {
		ImGui::End(); //end of dockspace window

		ImGuiIO& io = ImGui::GetIO();
		float time = glfwGetTime();
		io.DeltaTime = time > 0.0f ? (time - m_Time) : (1.0f / 60.0f);
		m_Time = time;

		ImGui::Render();
		UIPass();

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
	}

	void ImGuiLayer::UIPass() {
		auto currentArchitecture = Renderer::GetCurrentRenderArchitecture();

		if (currentArchitecture == RenderArchitecture::OpenGL) {
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		} else if (currentArchitecture == RenderArchitecture::Vulkan) {
			Renderer::Submit([this]() {
				for (uint32_t i = 0; i < VulkanRenderer::s_CommandPool->GetCommandBufferSize(); i++) {
					const auto& targetFrameBuffer = As(VulkanRenderer::m_GeometryPipeline->GetFrameBuffer(), VulkanFrameBuffer)->GetSwapChainFrameBuffers()[i];
					const auto& commandBuffer = VulkanRenderer::s_CommandPool->GetCommandBuffer(i);

					VkCommandBufferBeginInfo commandBufferBeginInfo{};
					commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
					LUCY_VK_ASSERT(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

					RenderPassBeginInfo beginInfo;
					beginInfo.CommandBuffer = commandBuffer;
					beginInfo.VulkanFrameBuffer = targetFrameBuffer;
					m_RenderPass->Begin(beginInfo);

					ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

					RenderPassEndInfo endInfo;
					endInfo.CommandBuffer = commandBuffer;
					endInfo.VulkanFrameBuffer = targetFrameBuffer;
					m_RenderPass->End(endInfo);
					LUCY_VK_ASSERT(vkEndCommandBuffer(commandBuffer));
				}
			});
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
			const ImGuiIO& io = ImGui::GetIO();
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
		auto currentArchitecture = Renderer::GetCurrentRenderArchitecture();
		if (currentArchitecture == RenderArchitecture::OpenGL) {
			ImGui_ImplOpenGL3_Shutdown();
		} else if (currentArchitecture == RenderArchitecture::Vulkan) {
			ImGui_ImplVulkan_Shutdown();
			m_ImGuiPool->Destroy();
			m_RenderPass->Destroy();
		}
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
}