#include "lypch.h"
#include "ImGuiOverlay.h"

#include "UI/SceneExplorerPanel.h"
#include "UI/TaskbarPanel.h"
#include "UI/ViewportPanel.h"
#include "UI/DetailsPanel.h"
#include "UI/ApplicationMetricsPanel.h"
#include "UI/ContentBrowserPanel.h"

#include "Core/Application.h"

#include "Renderer/Renderer.h"
#include "Renderer/RendererModule.h"

#include "Renderer/Context/VulkanContextDevice.h"
#include "Renderer/Context/VulkanContext.h"
#include "Renderer/Context/VulkanSwapChain.h"

#include "Renderer/Memory/Buffer/Vulkan/VulkanFrameBuffer.h"

namespace Lucy {

	/* --- Individual Resource Handles --- (TODO: abstract this maybe?) */
	static CommandResourceHandle g_ImGuiPassHandle;

	ImGuiOverlay::ImGuiOverlay() {
		m_Panels.push_back(&SceneExplorerPanel::GetInstance());
		m_Panels.push_back(&TaskbarPanel::GetInstance());
		m_Panels.push_back(&DetailsPanel::GetInstance());
		m_Panels.push_back(&ApplicationMetricsPanel::GetInstance());
		m_Panels.push_back(&ViewportPanel::GetInstance());
		m_Panels.push_back(&ContentBrowserPanel::GetInstance());

#pragma region ImGuiPipeline
		/*
		----ImGui (for Vulkan; does not need a separate pipeline)----
		*/
		if (Renderer::GetRenderArchitecture() == RenderArchitecture::Vulkan) {
			VulkanSwapChain& swapChain = VulkanSwapChain::Get();

			FrameBufferCreateInfo uiFrameBufferCreateInfo{
				.Width = (int32_t)swapChain.GetExtent().width,
				.Height = (int32_t)swapChain.GetExtent().height,
				.ImageViews = swapChain.GetImageViews()
			};

			RenderPassCreateInfo uiRenderPassCreateInfo;
			uiRenderPassCreateInfo.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
			uiRenderPassCreateInfo.Layout.ColorAttachments = {
				RenderPassLayout::Attachment {
					.Format = GetLucyImageFormat(swapChain.GetSurfaceFormat().format),
					.LoadOp = RenderPassLoadOp::Clear,
					.StoreOp = RenderPassStoreOp::Store,
					.StencilLoadOp = RenderPassLoadOp::DontCare,
					.StencilStoreOp = RenderPassStoreOp::DontCare,
					.Initial = VK_IMAGE_LAYOUT_UNDEFINED,
					.Final = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
					.Reference {
						.Layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
					},
				}
			};

			Ref<RenderPass> uiRenderPass = RenderPass::Create(uiRenderPassCreateInfo);
			uiFrameBufferCreateInfo.RenderPass = uiRenderPass;

			m_ImGuiPipeline.UIRenderPass = uiRenderPass;
			m_ImGuiPipeline.UIFramebuffer = FrameBuffer::Create(uiFrameBufferCreateInfo);
		}
#pragma endregion ImGuiPipeline
	}

	void ImGuiOverlay::Init(Ref<Window> window, Ref<Scene> scene, const Ref<RendererModule>& rendererModule) {
		auto& sceneExplorerPanel = SceneExplorerPanel::GetInstance();
		auto& viewportPanel = ViewportPanel::GetInstance();

		sceneExplorerPanel.SetScene(scene);
		sceneExplorerPanel.SetIDPipeline(rendererModule->GetIDPipeline());

		viewportPanel.SetViewportOutputPipeline(rendererModule->GetGeometryPipeline());
		viewportPanel.SetOnViewportResizeCallback([rendererModule]() { rendererModule->OnViewportResize(); });

		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.IniFilename = "lucyconfig.ini";

		io.Fonts->AddFontFromFileTTF("Assets/Fonts/ComicMono.ttf", 13);

		int32_t width, height;
		glfwGetWindowSize(window->Raw(), &width, &height);
		io.DisplaySize = { (float)width, (float)height };

		if (Renderer::GetRenderArchitecture() == RenderArchitecture::Vulkan) {
			ImGui_ImplGlfw_InitForVulkan(window->Raw(), true);

			m_ImGuiPool = Memory::CreateRef<VulkanDescriptorPool>(m_PoolSpecs);

			Renderer::EnqueueToRenderThread([&]() mutable {
				VulkanSwapChain& swapChain = VulkanSwapChain::Get();
				const auto& vulkanContext = Renderer::GetRenderContext().As<VulkanContext>();
				VulkanContextDevice device = VulkanContextDevice::Get();

				ImGui_ImplVulkan_InitInfo initInfo{};
				initInfo.Instance = vulkanContext->GetVulkanInstance();
				initInfo.PhysicalDevice = device.GetPhysicalDevice();
				initInfo.GPUDevice = device.GetLogicalDevice();
				initInfo.Queue = device.GetGraphicsQueue();
				initInfo.DescriptorPool = m_ImGuiPool->GetVulkanHandle();
				initInfo.MinImageCount = Renderer::GetMaxFramesInFlight();
				initInfo.ImageCount = initInfo.MinImageCount;
				initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
				initInfo.CheckVkResultFn = VulkanMessageCallback::ImGui_DebugCallback;

				ImGui_ImplVulkan_Init(&initInfo, m_ImGuiPipeline.UIRenderPass.As<VulkanRenderPass>()->GetVulkanHandle());
				Renderer::SubmitImmediateCommand(ImGui_ImplVulkan_CreateFontsTexture);
				ImGui_ImplVulkan_DestroyFontUploadObjects();
			});

			g_ImGuiPassHandle = Renderer::CreateCommandResource(nullptr, [this](void* commandBuffer, Ref<GraphicsPipeline> unused, RenderCommand* unusedC) {
				const auto& renderPass = m_ImGuiPipeline.UIRenderPass.As<VulkanRenderPass>();
				const auto& frameBufferHandle = m_ImGuiPipeline.UIFramebuffer.As<VulkanFrameBuffer>();
				const auto& targetFrameBuffer = frameBufferHandle->GetVulkanHandles()[Renderer::GetCurrentImageIndex()];

				VulkanRenderPassBeginInfo beginInfo;
				beginInfo.Width = frameBufferHandle->GetWidth();
				beginInfo.Height = frameBufferHandle->GetHeight();
				beginInfo.CommandBuffer = (VkCommandBuffer)commandBuffer;
				beginInfo.VulkanFrameBuffer = targetFrameBuffer;

				renderPass->Begin(beginInfo);
				ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), (VkCommandBuffer)commandBuffer);
				renderPass->End();
			});
		}
	}

	void ImGuiOverlay::Begin() {
		LUCY_PROFILE_NEW_EVENT("ImGuiOverlay::Begin");

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

	void ImGuiOverlay::End() {
		LUCY_PROFILE_NEW_EVENT("ImGuiOverlay::End");

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
	}

	void ImGuiOverlay::Render() {
		LUCY_PROFILE_NEW_EVENT("ImGuiOverlay::Render");

		Begin();
		for (Panel* panel : m_Panels) {
			panel->Render();
		}
		End();
	}

	void ImGuiOverlay::SendImGuiDataToDevice() {
		LUCY_PROFILE_NEW_EVENT("ImGuiOverlay::SendImGuiDataToDevice");

		if (Renderer::GetRenderArchitecture() == RenderArchitecture::Vulkan)
			Renderer::EnqueueCommand<ImGuiRenderCommand>(g_ImGuiPassHandle);
	}

	void ImGuiOverlay::OnEvent(Event& e) {
		auto& inputHandler = Application::Get()->GetInputHandler();

		inputHandler.Dispatch<ScrollEvent>(e, EventType::ScrollEvent, [&](ScrollEvent& e) {
			ImGuiIO& io = ImGui::GetIO();
			io.AddMouseWheelEvent((float)e.GetXOffset(), (float)e.GetYOffset());
		});

		inputHandler.Dispatch<CursorPosEvent>(e, EventType::CursorPosEvent, [&](CursorPosEvent& e) {
			auto& inputHandler = Application::Get()->GetInputHandler();
			inputHandler.MouseX = e.GetXPos();
			inputHandler.MouseY = e.GetYPos();
			ImGui_ImplGlfw_CursorPosCallback(e.GetWindowHandle(), inputHandler.MouseX, inputHandler.MouseY);
		});

		inputHandler.Dispatch<MouseEvent>(e, EventType::MouseEvent, [&](MouseEvent& e) {
			ImGui_ImplGlfw_MouseButtonCallback(e.GetWindowHandle(), e.GetButton(), e.GetAction(), e.GetMods());
		});

		inputHandler.Dispatch<KeyEvent>(e, EventType::KeyEvent, [&](KeyEvent& e) {
			ImGui_ImplGlfw_KeyCallback(e.GetWindowHandle(), e.GetKey(), e.GetScanCode(), e.GetAction(), e.GetMods());
		});

		inputHandler.Dispatch<CharCallbackEvent>(e, EventType::CharCallbackEvent, [&](CharCallbackEvent& e) {
			ImGui_ImplGlfw_CharCallback(e.GetWindowHandle(), e.GetCodePoint());
		});

		inputHandler.Dispatch<WindowResizeEvent>(e, EventType::WindowResizeEvent, [&](WindowResizeEvent& e) {
			ImGuiIO& io = ImGui::GetIO();
			io.DisplaySize = { (float)e.GetWidth(), (float)e.GetHeight() };
			io.DisplayFramebufferScale = { 1.0f, 1.0f };

			Renderer::EnqueueToRenderThread([this]() {
				if (Renderer::GetRenderArchitecture() == RenderArchitecture::Vulkan) {
					VulkanSwapChain& swapChain = VulkanSwapChain::Get();

					const auto& extent = swapChain.GetExtent();
					//m_ImGuiPipeline.UIRenderPass->Recreate(); being done on framebuffer recreate (swapchain ONLY)
					m_ImGuiPipeline.UIFramebuffer.As<VulkanFrameBuffer>()->Recreate(extent.width, extent.height, swapChain.GetImageViews());
				}
			});
		});

		for (Panel* panel : m_Panels) {
			panel->OnEvent(e);
		}
	}

	void ImGuiOverlay::Destroy() {
		for (Panel* panel : m_Panels)
			panel->OnDestroy();

		auto currentArchitecture = Renderer::GetRenderArchitecture();
		if (currentArchitecture == RenderArchitecture::Vulkan) {
			ImGui_ImplVulkan_Shutdown();
			m_ImGuiPool->Destroy();
		}

		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		m_ImGuiPipeline.UIFramebuffer->Destroy();
		m_ImGuiPipeline.UIRenderPass->Destroy();
	}
}