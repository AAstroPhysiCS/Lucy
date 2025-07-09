#include "lypch.h"
#include "ImGuiPass.h"

#include "Context/VulkanContext.h"

#include "../../ThirdParty/ImGui/imgui_impl_vulkan.h"
#include "../../ThirdParty/ImGui/imgui_impl_glfw.h"

namespace Lucy {

	void ImGuiVulkanImpl::Init(RendererBackend* backend) {
		const auto& vulkanContext = backend->GetRenderContext()->As<VulkanContext>();
		const auto& vulkanDevice = backend->GetRenderDevice()->As<VulkanRenderDevice>();
		const auto& swapChain = backend->GetSwapChain()->As<VulkanSwapChain>();
		const auto& window = vulkanContext->GetWindow();

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

#if USE_INTEGRATED_GRAPHICS
		static constexpr auto IMGUI_MAX_POOL_SIZES = 100u;
#else
		static constexpr auto IMGUI_MAX_POOL_SIZES = 100u;
#endif

		VulkanDescriptorPoolCreateInfo PoolSpecs = { 
			.PoolSizesVector = {
				{ .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = IMGUI_MAX_POOL_SIZES },
			}, 
			.PoolFlags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 
			.MaxSet = IMGUI_MAX_POOL_SIZES,
			.LogicalDevice = vulkanDevice->GetLogicalDevice()
		};

		ImGuiPool = Memory::CreateRef<VulkanDescriptorPool>(PoolSpecs);

		ImGui_ImplVulkan_InitInfo initInfo{};
		initInfo.Instance = vulkanContext->GetVulkanInstance();
		initInfo.PhysicalDevice = vulkanDevice->GetPhysicalDevice();
		initInfo.Device = vulkanDevice->GetLogicalDevice();
		initInfo.Queue = vulkanDevice->GetGraphicsQueue();
		initInfo.DescriptorPool = ImGuiPool->GetVulkanHandle();
		initInfo.MinImageCount = (uint32_t)swapChain->GetSwapChainImageCount();
		initInfo.ImageCount = initInfo.MinImageCount;
		initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		initInfo.CheckVkResultFn = VulkanMessageCallback::ImGui_DebugCallback;
		initInfo.RenderPass = swapChain->GetRenderPass()->GetVulkanHandle();

		LUCY_ASSERT(ImGui_ImplGlfw_InitForVulkan(window->Raw(), true), "Vulkan GLFW ImGui initialization failed!");
		LUCY_ASSERT(ImGui_ImplVulkan_Init(&initInfo), "Vulkan ImGui initialization failed!");
		Renderer::SubmitImmediateCommand([](VkCommandBuffer cmd) { ImGui_ImplVulkan_CreateFontsTexture(); });
	}

	void ImGuiVulkanImpl::Render(const Ref<VulkanSwapChain>& swapChain, RenderCommandList& cmdList) {
		LUCY_PROFILE_NEW_EVENT("ImGuiVulkanImpl::Render");
		const auto& renderPass = swapChain->GetRenderPass();
		const auto& frameBuffer = swapChain->GetFrameBuffer();

		VulkanRenderPassBeginInfo beginInfo;
		beginInfo.Width = frameBuffer->GetWidth();
		beginInfo.Height = frameBuffer->GetHeight();
		beginInfo.CommandBuffer = (VkCommandBuffer)cmdList.GetPrimaryCommandPool()->GetCurrentFrameCommandBuffer();
		beginInfo.VulkanFrameBuffer = frameBuffer->GetVulkanHandles()[Renderer::GetCurrentImageIndex()];
		auto& cmd = cmdList.BeginRenderCommand("ImGuiPass");
		renderPass->RTBegin(beginInfo);
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), beginInfo.CommandBuffer);
		renderPass->RTEnd();
		cmdList.EndRenderCommand();
	}

	void ImGuiVulkanImpl::Destroy() {
		//Framebuffer and renderpass is destroyed once swapchain is destroyed and is also getting properly resized once swapchain resizes.
		ImGuiPool->RTDestroyResource();
	}
}