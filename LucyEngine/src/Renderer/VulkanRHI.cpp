#include "lypch.h"
#include "VulkanRHI.h"

#include "Context/RenderContext.h"
#include "Context/VulkanPipeline.h"
#include "Context/VulkanSwapChain.h"
#include "Context/VulkanDevice.h"
#include "Memory/Buffer/Vulkan/VulkanFrameBuffer.h"

#include "Scene/Scene.h"
#include "Scene/Entity.h"

#include "ViewportRenderer.h"
#include "Renderer/Renderer.h"

#include "Utils.h"

namespace Lucy {

	VulkanRHI::VulkanRHI(RenderArchitecture arch)
		: RHI(arch) {
		m_RenderContext = RenderContext::Create(arch);
		m_RenderContext->PrintInfo();
	}

	void VulkanRHI::Init() {
		s_CommandQueue.Init();
	}

	void VulkanRHI::Dispatch() {
		uint32_t oldSize = m_RenderFunctionQueue.size();
		for (uint32_t i = 0; i < oldSize; i++) {
			m_RenderFunctionQueue[i](); //functions can contain nested functions
		}
		//meaing that nested lambda functions are being run in the second iteration.
		m_RenderFunctionQueue.erase(m_RenderFunctionQueue.begin(), m_RenderFunctionQueue.begin() + oldSize);
	}

	void VulkanRHI::BeginScene(Scene& scene) {
		m_ActiveScene = &scene;
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		swapChain.BeginFrame();
	}

	void VulkanRHI::RenderScene() {
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		VkResult lastSwapChainResult = swapChain.GetLastSwapChainResult();
		if (lastSwapChainResult == VK_ERROR_OUT_OF_DATE_KHR || lastSwapChainResult == VK_SUBOPTIMAL_KHR)
			return;
		s_CommandQueue.Execute();
	}

	PresentResult VulkanRHI::EndScene() {
		//TODO: profiling
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		swapChain.EndFrame(s_CommandQueue);
		return (PresentResult)swapChain.Present();
	}

	// Should not be used in a loop 
	void VulkanRHI::DirectCopyBuffer(VkBuffer& stagingBuffer, VkBuffer& buffer, VkDeviceSize size) {
		RecordSingleTimeCommand([&](VkCommandBuffer commandBuffer) {
			VkBufferCopy copyRegion{};
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = 0;
			copyRegion.size = size;
			vkCmdCopyBuffer(commandBuffer, stagingBuffer, buffer, 1, &copyRegion);
		});
	}

	void VulkanRHI::Enqueue(const SubmitFunc&& func) {
		m_RenderFunctionQueue.push_back(func);
	}

	void VulkanRHI::EnqueueStaticMesh(Priority priority, Ref<Mesh> mesh, const glm::mat4& entityTransform) {
		m_StaticMeshDrawCommands.push_back(Memory::CreateRef<MeshDrawCommand>(priority, mesh, entityTransform));
	}

	void VulkanRHI::RecordStaticMeshToCommandQueue(Ref<Pipeline> pipeline, RecordFunc<Ref<DrawCommand>>&& func) {
		CommandElement element;
		element.Pipeline = pipeline;
		element.RecordFunc = func;
		element.Arguments = m_StaticMeshDrawCommands;

		s_CommandQueue.Enqueue(element);
	}

	void VulkanRHI::BindBuffers(Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer) {
		VertexBindInfo vertexInfo;
		vertexInfo.CommandBuffer = s_CommandQueue.GetCurrentCommandBuffer();
		vertexBuffer->Bind(vertexInfo);

		IndexBindInfo indexInfo;
		indexInfo.CommandBuffer = vertexInfo.CommandBuffer;
		indexBuffer->Bind(indexInfo);
	}

	void VulkanRHI::RecordSingleTimeCommand(std::function<void(VkCommandBuffer)>&& func) {
		const VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		VkCommandBuffer commandBuffer = s_CommandQueue.BeginSingleTimeCommand();
		func(commandBuffer);
		s_CommandQueue.EndSingleTimeCommand(commandBuffer);
	}

	void VulkanRHI::Destroy() {
		s_CommandQueue.Free();
		m_RenderContext->Destroy();
	}

	void VulkanRHI::OnWindowResize() {
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		swapChain.Recreate();

		s_CommandQueue.Recreate();

		ViewportRenderer::s_GeometryPipeline.As<VulkanPipeline>()->Recreate(m_ViewportWidth, m_ViewportHeight);

		ViewportRenderer::s_ImGuiPipeline.UIRenderPass->Recreate();

		auto& extent = swapChain.GetExtent();
		auto& desc = swapChain.GetSwapChainFrameBufferDesc();
		desc->RenderPass = ViewportRenderer::s_ImGuiPipeline.UIRenderPass;

		ViewportRenderer::s_ImGuiPipeline.UIFramebuffer.As<VulkanFrameBuffer>()->Recreate(extent.width, extent.height, desc);
	}

	void VulkanRHI::OnViewportResize() {
		const VulkanDevice& device = VulkanDevice::Get();
		vkDeviceWaitIdle(device.GetLogicalDevice());

		ViewportRenderer::s_GeometryPipeline.As<VulkanPipeline>()->Recreate(m_ViewportWidth, m_ViewportHeight);
	}

	Entity VulkanRHI::OnMousePicking() {
		return {};
	}

	void VulkanRHI::UIPass(const ImGuiPipeline& imguiPipeline, std::function<void(VkCommandBuffer commandBuffer)>&& imguiRenderFunc) {
		//we won't be using the drawcommand here, it's empty
		RecordFunc<Ref<DrawCommand>> recordFunc = [=](Ref<DrawCommand>) {
			VkCommandBuffer commandBuffer = s_CommandQueue.GetCurrentCommandBuffer();
			VulkanSwapChain& swapChain = VulkanSwapChain::Get();

			auto& renderPass = imguiPipeline.UIRenderPass.As<VulkanRenderPass>();
			auto& frameBufferHandle = imguiPipeline.UIFramebuffer.As<VulkanFrameBuffer>();
			const auto& targetFrameBuffer = frameBufferHandle->GetVulkanHandles()[swapChain.GetCurrentImageIndex()];

			RenderPassBeginInfo beginInfo;
			beginInfo.Width = frameBufferHandle->GetWidth();
			beginInfo.Height = frameBufferHandle->GetHeight();
			beginInfo.CommandBuffer = commandBuffer;
			beginInfo.VulkanFrameBuffer = targetFrameBuffer;

			renderPass->Begin(beginInfo);
			imguiRenderFunc(commandBuffer);
			renderPass->End();
		};

		CommandElement imguiElement;
		imguiElement.RecordFunc = recordFunc;

		s_CommandQueue.Enqueue(imguiElement);
	}
}