#include "lypch.h"
#include "ImGuiPass.h"

#include "Context/VulkanContext.h"

#include "../../ThirdParty/ImGui/imgui_impl_vulkan.h"
#include "../../ThirdParty/ImGui/imgui_impl_glfw.h"

namespace Lucy {

	void ImGuiVulkanImpl::Init(const Ref<RenderContext>& renderContext) {
		const auto& vulkanContext = renderContext->As<VulkanContext>();
		const auto& swapChain = vulkanContext->GetSwapChain();
		const auto& vulkanDevice = vulkanContext->GetRenderDevice()->As<VulkanRenderDevice>();

		PoolSpecs.LogicalDevice = vulkanDevice->GetLogicalDevice();
		ImGuiPool = Memory::CreateRef<VulkanDescriptorPool>(PoolSpecs);

		ImGui_ImplVulkan_InitInfo initInfo{};
		initInfo.Instance = vulkanContext->GetVulkanInstance();
		initInfo.PhysicalDevice = vulkanDevice->GetPhysicalDevice();
		initInfo.GPUDevice = vulkanDevice->GetLogicalDevice();
		initInfo.Queue = vulkanDevice->GetGraphicsQueue();
		initInfo.DescriptorPool = ImGuiPool->GetVulkanHandle();
		initInfo.MinImageCount = (uint32_t)vulkanContext->GetSwapChain().GetSwapChainImageCount();
		initInfo.ImageCount = initInfo.MinImageCount;
		initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		initInfo.CheckVkResultFn = VulkanMessageCallback::ImGui_DebugCallback;

		LUCY_ASSERT(ImGui_ImplGlfw_InitForVulkan(renderContext->GetWindow()->Raw(), true), "Vulkan GLFW ImGui initialization failed!");
		LUCY_ASSERT(ImGui_ImplVulkan_Init(&initInfo, swapChain.GetRenderPass()->GetVulkanHandle()), "Vulkan ImGui initialization failed!");
		Renderer::SubmitImmediateCommand(ImGui_ImplVulkan_CreateFontsTexture);
		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

	void ImGuiVulkanImpl::Render(const VulkanSwapChain& swapChain, const RenderCommandList& cmdList) {
		LUCY_PROFILE_NEW_EVENT("ImGuiVulkanImpl::Render");
		const auto& renderPass = swapChain.GetRenderPass();
		const auto& frameBuffer = swapChain.GetFrameBuffer();

		VulkanRenderPassBeginInfo beginInfo;
		beginInfo.Width = frameBuffer->GetWidth();
		beginInfo.Height = frameBuffer->GetHeight();
		beginInfo.CommandBuffer = (VkCommandBuffer)cmdList.GetPrimaryCommandPool()->GetCurrentFrameCommandBuffer();
		beginInfo.VulkanFrameBuffer = frameBuffer->GetVulkanHandles()[Renderer::GetCurrentImageIndex()];

		renderPass->RTBegin(beginInfo);
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), beginInfo.CommandBuffer);
		renderPass->RTEnd();
	}

	void ImGuiVulkanImpl::Destroy() {
		//Framebuffer and renderpass is destroyed once swapchain is destroyed and is also getting properly resized once swapchain resizes.
		ImGuiPool->RTDestroyResource();
	}
}