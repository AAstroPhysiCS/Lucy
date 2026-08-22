#include "lypch.h"
#include "ImGuiPass.h"

#include "Context/VulkanContext.h"

#include "Renderer.h"
#include "RendererBackend.h"

#include "Descriptors/VulkanDescriptorPool.h"

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

		static constexpr auto IMGUI_MAX_POOL_SIZES = 1000u;

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
		initInfo.Queue = vulkanDevice->GetQueue(TargetQueueFamily::Graphics).Handle;
		initInfo.DescriptorPool = ImGuiPool->GetVulkanHandle();
		initInfo.MinImageCount = (uint32_t)swapChain->GetSwapChainImageCount();
		initInfo.ImageCount = initInfo.MinImageCount;
		initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		initInfo.CheckVkResultFn = VulkanMessageCallback::ImGui_DebugCallback;
		initInfo.RenderPass = swapChain->GetRenderPass()->GetVulkanHandle();

		LUCY_ASSERT(ImGui_ImplGlfw_InitForVulkan(window->Raw(), true), "Vulkan GLFW ImGui initialization failed!");
		LUCY_ASSERT(ImGui_ImplVulkan_Init(&initInfo), "Vulkan ImGui initialization failed!");
		Renderer::SubmitImmediateCommand([](VkCommandBuffer cmd) { 
			ImGui_ImplVulkan_CreateFontsTexture(); 

			/*VkMemoryBarrier2* barrier2 = new VkMemoryBarrier2();
			barrier2->sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
			barrier2->srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
			barrier2->srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			barrier2->dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
			barrier2->dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;

			VkDependencyInfo* depInfo2 = new VkDependencyInfo();
			depInfo2->sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			depInfo2->memoryBarrierCount = 1;
			depInfo2->pMemoryBarriers = barrier2;

			vkCmdPipelineBarrier2(cmd, depInfo2);*/
		});
	}

	void ImGuiVulkanImpl::Render(const Ref<VulkanSwapChain>& swapChain, RenderCommandList& cmdList) {
		LUCY_PROFILE_NEW_EVENT("ImGuiVulkanImpl::Render");
		const auto& renderPass = swapChain->GetRenderPass();
		const auto& frameBuffer = swapChain->GetFrameBuffer();

		const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		const uint32_t imageIndex = Renderer::GetCurrentImageIndex();

		VulkanRenderPassBeginInfo beginInfo;
		beginInfo.Width = frameBuffer->GetWidth();
		beginInfo.Height = frameBuffer->GetHeight();
		beginInfo.CommandBuffer = (VkCommandBuffer)cmdList.GetPrimaryCommandPool()->GetCommandBuffer(frameIndex);
		beginInfo.VulkanFrameBuffer = frameBuffer->GetVulkanHandles()[imageIndex];

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