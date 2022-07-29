#include "lypch.h"
#include "ImGuiOverlay.h"

#include "UI/SceneExplorerPanel.h"
#include "UI/TaskbarPanel.h"
#include "UI/ViewportPanel.h"
#include "UI/DetailsPanel.h"
#include "UI/ApplicationMetricsPanel.h"

#include "Core/Input.h"

#include "Renderer/Renderer.h"
#include "Renderer/RendererModule.h"

#include "Renderer/Context/VulkanDevice.h"
#include "Renderer/Context/VulkanContext.h"
#include "Renderer/Context/VulkanSwapChain.h"
#include "Renderer/VulkanRenderPass.h"

#include "Renderer/Memory/Buffer/Vulkan/VulkanFrameBuffer.h"

namespace Lucy {

	/* --- Individual Resource Handles --- (TODO: abstract this maybe?) */
	static RenderCommandResourceHandle g_ImGuiPassHandle;

	ImGuiOverlay::ImGuiOverlay() {
		m_Panels.push_back(&SceneExplorerPanel::GetInstance());
		m_Panels.push_back(&TaskbarPanel::GetInstance());
		m_Panels.push_back(&DetailsPanel::GetInstance());
		m_Panels.push_back(&ApplicationMetricsPanel::GetInstance());
		m_Panels.push_back(&ViewportPanel::GetInstance());

#pragma region ImGuiPipeline
		/*
		----ImGui (for Vulkan; does not need a separate pipeline)----
		*/
		if (Renderer::GetRenderArchitecture() == RenderArchitecture::Vulkan) {
			VulkanSwapChain& swapChain = VulkanSwapChain::Get();

			RenderPassCreateInfo uiRenderPassCreateInfo;
			uiRenderPassCreateInfo.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

			Ref<VulkanRenderPassInfo> vulkanRenderPassInfo = Memory::CreateRef<VulkanRenderPassInfo>();
			vulkanRenderPassInfo->ColorAttachments.push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
			vulkanRenderPassInfo->ColorDescriptor.Format = swapChain.GetSurfaceFormat().format;
			vulkanRenderPassInfo->ColorDescriptor.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			vulkanRenderPassInfo->ColorDescriptor.StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
			vulkanRenderPassInfo->ColorDescriptor.StencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			vulkanRenderPassInfo->ColorDescriptor.StencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			vulkanRenderPassInfo->ColorDescriptor.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			vulkanRenderPassInfo->ColorDescriptor.FinalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			uiRenderPassCreateInfo.InternalInfo = vulkanRenderPassInfo;
			m_ImGuiPipeline.UIRenderPass = RenderPass::Create(uiRenderPassCreateInfo);

			FrameBufferCreateInfo uiFrameBufferCreateInfo;
			uiFrameBufferCreateInfo.Width = swapChain.GetExtent().width;
			uiFrameBufferCreateInfo.Height = swapChain.GetExtent().height;

			Ref<VulkanFrameBufferInfo> vulkanFrameBufferInfo = Memory::CreateRef<VulkanFrameBufferInfo>();
			vulkanFrameBufferInfo->ImageViews = swapChain.GetImageViews();
			vulkanFrameBufferInfo->RenderPass = m_ImGuiPipeline.UIRenderPass;

			uiFrameBufferCreateInfo.InternalInfo = vulkanFrameBufferInfo;
			m_ImGuiPipeline.UIFramebuffer = FrameBuffer::Create(uiFrameBufferCreateInfo);
		}
#pragma endregion ImGuiPipeline
	}

	void ImGuiOverlay::Init(Ref<Window> window, Ref<Scene> scene, const Ref<RendererModule>& rendererModule) {
		SceneExplorerPanel::GetInstance().SetScene(scene);
		
		SceneExplorerPanel::GetInstance().SetIDPipeline(rendererModule->GetIDPipeline());
		ViewportPanel::GetInstance().SetViewportOutputPipeline(rendererModule->GetGeometryPipeline());

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
				auto& vulkanContext = Renderer::GetRenderContext().As<VulkanContext>();
				VulkanDevice device = VulkanDevice::Get();

				ImGui_ImplVulkan_InitInfo initInfo{};
				initInfo.Instance = vulkanContext->GetVulkanInstance();
				initInfo.PhysicalDevice = device.GetPhysicalDevice();
				initInfo.GPUDevice = device.GetLogicalDevice();
				initInfo.Queue = device.GetGraphicsQueue();
				initInfo.DescriptorPool = m_ImGuiPool->GetVulkanHandle();
				initInfo.MinImageCount = swapChain.GetImageCount();
				initInfo.ImageCount = swapChain.GetImageCount();
				initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
				initInfo.CheckVkResultFn = VulkanMessageCallback::ImGui_DebugCallback;

				ImGui_ImplVulkan_Init(&initInfo, m_ImGuiPipeline.UIRenderPass.As<VulkanRenderPass>()->GetVulkanHandle());
				Renderer::ExecuteSingleTimeCommand(ImGui_ImplVulkan_CreateFontsTexture);
				ImGui_ImplVulkan_DestroyFontUploadObjects();
			});

			g_ImGuiPassHandle = Renderer::CreateRenderPassResource([this](void* commandBuffer, Ref<Pipeline> unused, RenderCommand* unusedC) {
				VulkanSwapChain& swapChain = VulkanSwapChain::Get();

				auto& renderPass = m_ImGuiPipeline.UIRenderPass.As<VulkanRenderPass>();
				auto& frameBufferHandle = m_ImGuiPipeline.UIFramebuffer.As<VulkanFrameBuffer>();
				const auto& targetFrameBuffer = frameBufferHandle->GetVulkanHandles()[swapChain.GetCurrentImageIndex()];

				RenderPassBeginInfo beginInfo;
				beginInfo.Width = frameBufferHandle->GetWidth();
				beginInfo.Height = frameBufferHandle->GetHeight();
				beginInfo.CommandBuffer = (VkCommandBuffer)commandBuffer;
				beginInfo.VulkanFrameBuffer = targetFrameBuffer;

				renderPass->Begin(beginInfo);
				ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), (VkCommandBuffer)commandBuffer);
				renderPass->End();
			}, nullptr);
		}
	}

	void ImGuiOverlay::Begin() {
		auto currentArchitecture = Renderer::GetRenderArchitecture();
		if (currentArchitecture == RenderArchitecture::Vulkan) {
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

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
	}

	void ImGuiOverlay::SendImGuiDataToDevice() {
		if (Renderer::GetRenderArchitecture() == RenderArchitecture::Vulkan)
			Renderer::EnqueueRenderCommand<ImGuiRenderCommand>(g_ImGuiPassHandle);
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

			Renderer::EnqueueToRenderThread([this]() {
				if (Renderer::GetRenderArchitecture() == RenderArchitecture::Vulkan) {
					VulkanSwapChain& swapChain = VulkanSwapChain::Get();

					m_ImGuiPipeline.UIRenderPass->Recreate();

					auto& extent = swapChain.GetExtent();
					auto& desc = swapChain.GetSwapChainFrameBufferInfo();
					desc->RenderPass = m_ImGuiPipeline.UIRenderPass;

					m_ImGuiPipeline.UIFramebuffer->Recreate(extent.width, extent.height, desc);
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