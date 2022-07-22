#include "lypch.h"
#include "VulkanRenderDevice.h"

#include "Context/RenderContext.h"
#include "Context/VulkanPipeline.h"
#include "Context/VulkanSwapChain.h"
#include "Context/VulkanDevice.h"
#include "Descriptors/VulkanDescriptorSet.h"
#include "Commands/VulkanCommandQueue.h"
#include "Memory/Buffer/Vulkan/VulkanFrameBuffer.h"

#include "Scene/Scene.h"
#include "Scene/Entity.h"

#include "ViewportRenderer.h"
#include "Renderer.h"

#include "Utils/Utils.h"

namespace Lucy {

	VulkanRenderDevice::VulkanRenderDevice(RenderArchitecture arch)
		: RenderDevice(arch) {
		m_RenderContext = RenderContext::Create(arch);
		m_RenderContext->PrintInfo();

		s_CommandQueue = CommandQueue::Create();
	}

	void VulkanRenderDevice::Init() {
		s_CommandQueue->Init();
	}

	void VulkanRenderDevice::BeginScene(Scene& scene) {
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		swapChain.BeginFrame();
	}

	void VulkanRenderDevice::RenderScene() {
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		VkResult lastSwapChainResult = swapChain.GetLastSwapChainResult();
		if (lastSwapChainResult == VK_ERROR_OUT_OF_DATE_KHR || lastSwapChainResult == VK_SUBOPTIMAL_KHR)
			return;
		s_CommandQueue->Execute();
	}

	PresentResult VulkanRenderDevice::EndScene() {
		//TODO: profiling with optick
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		swapChain.EndFrame(s_CommandQueue);
		return (PresentResult)swapChain.Present();
	}

	void VulkanRenderDevice::Wait() {
		VkDevice device = VulkanDevice::Get().GetLogicalDevice();
		LUCY_VK_ASSERT(vkDeviceWaitIdle(device));
	}

	void VulkanRenderDevice::BindBuffers(void* commandBufferHandle, Ref<Mesh> mesh) {
		VertexBindInfo vertexInfo;
		vertexInfo.CommandBuffer = (VkCommandBuffer) commandBufferHandle;
		mesh->GetVertexBuffer()->Bind(vertexInfo);

		IndexBindInfo indexInfo;
		indexInfo.CommandBuffer = vertexInfo.CommandBuffer;
		mesh->GetIndexBuffer()->Bind(indexInfo);
	}

	void VulkanRenderDevice::BindPushConstant(void* commandBufferHandle, Ref<Pipeline> pipeline, const PushConstant& pushConstant) {
		PushConstantBindInfo bindInfo;
		bindInfo.CommandBuffer = (VkCommandBuffer)commandBufferHandle;
		bindInfo.PipelineLayout = pipeline.As<VulkanPipeline>()->GetPipelineLayout();
		pushConstant.Bind(bindInfo);
	}

	void VulkanRenderDevice::BindPipeline(void* commandBufferHandle, Ref<Pipeline> pipeline) {
		PipelineBindInfo info;
		info.PipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		info.CommandBuffer = (VkCommandBuffer)commandBufferHandle;

		pipeline->Bind(info);
	}

	void VulkanRenderDevice::BindDescriptorSet(void* commandBufferHandle, Ref<Pipeline> pipeline, Ref<DescriptorSet> descriptorSet) {
		Ref<VulkanPipeline> vulkanPipeline = pipeline.As<VulkanPipeline>();
		Ref<VulkanDescriptorSet> vulkanSet = descriptorSet.As<VulkanDescriptorSet>();

		VulkanDescriptorSetBindInfo bindInfo;
		bindInfo.CommandBuffer = (VkCommandBuffer)commandBufferHandle;
		bindInfo.PipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		bindInfo.PipelineLayout = vulkanPipeline->GetPipelineLayout();

		vulkanSet->Update();
		vulkanSet->Bind(bindInfo);
	}

	void VulkanRenderDevice::BindBuffers(void* commandBufferHandle, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer) {
		VertexBindInfo vertexInfo;
		vertexInfo.CommandBuffer = (VkCommandBuffer)commandBufferHandle;
		vertexBuffer->Bind(vertexInfo);

		IndexBindInfo indexInfo;
		indexInfo.CommandBuffer = vertexInfo.CommandBuffer;
		indexBuffer->Bind(indexInfo);
	}

	void VulkanRenderDevice::DrawIndexed(void* commandBufferHandle, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
		vkCmdDrawIndexed((VkCommandBuffer) commandBufferHandle, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void VulkanRenderDevice::BeginRenderPass(void* commandBufferHandle, Ref<Pipeline> pipeline) {
		Ref<RenderPass> renderPass = pipeline->GetRenderPass();
		Ref<FrameBuffer> frameBuffer = pipeline->GetFrameBuffer();

		RenderPassBeginInfo renderPassBeginInfo;
		renderPassBeginInfo.CommandBuffer = (VkCommandBuffer) commandBufferHandle;
		renderPassBeginInfo.Width = frameBuffer->GetWidth();
		renderPassBeginInfo.Height = frameBuffer->GetHeight();

		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		renderPassBeginInfo.VulkanFrameBuffer = frameBuffer.As<VulkanFrameBuffer>()->GetVulkanHandles()[swapChain.GetCurrentFrameIndex()];

		renderPass->Begin(renderPassBeginInfo);
	}

	void VulkanRenderDevice::EndRenderPass(Ref<Pipeline> pipeline) {
		pipeline->GetRenderPass()->End();
	}

	// Should not be used in a loop 
	void VulkanRenderDevice::DirectCopyBuffer(VkBuffer& stagingBuffer, VkBuffer& buffer, VkDeviceSize size) {
		RecordSingleTimeCommand([&](VkCommandBuffer commandBuffer) {
			VkBufferCopy copyRegion{};
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = 0;
			copyRegion.size = size;
			vkCmdCopyBuffer(commandBuffer, stagingBuffer, buffer, 1, &copyRegion);
		});
	}

	void VulkanRenderDevice::RecordSingleTimeCommand(std::function<void(VkCommandBuffer)>&& func) {
		const VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		VkCommandBuffer commandBuffer = s_CommandQueue.As<VulkanCommandQueue>()->BeginSingleTimeCommand();
		func(commandBuffer);
		s_CommandQueue.As<VulkanCommandQueue>()->EndSingleTimeCommand(commandBuffer);
	}

	void VulkanRenderDevice::Destroy() {
		s_CommandQueue->Free();
		m_RenderContext->Destroy();
	}

	void VulkanRenderDevice::OnWindowResize() {
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		swapChain.Recreate();

		s_CommandQueue->Recreate();

		ViewportRenderer::s_GeometryPipeline.As<VulkanPipeline>()->Recreate(m_ViewportWidth, m_ViewportHeight);

		ViewportRenderer::s_ImGuiPipeline.UIRenderPass->Recreate();

		auto& extent = swapChain.GetExtent();
		auto& desc = swapChain.GetSwapChainFrameBufferInfo();
		desc->RenderPass = ViewportRenderer::s_ImGuiPipeline.UIRenderPass;

		ViewportRenderer::s_ImGuiPipeline.UIFramebuffer.As<VulkanFrameBuffer>()->Recreate(extent.width, extent.height, desc);
	}

	void VulkanRenderDevice::OnViewportResize() {
		const VulkanDevice& device = VulkanDevice::Get();
		vkDeviceWaitIdle(device.GetLogicalDevice());

		ViewportRenderer::s_GeometryPipeline.As<VulkanPipeline>()->Recreate(m_ViewportWidth, m_ViewportHeight);
	}

	Entity VulkanRenderDevice::OnMousePicking() {
		return {};
	}

	void VulkanRenderDevice::UIPass(const ImGuiPipeline& imguiPipeline, std::function<void(VkCommandBuffer commandBuffer)>&& imguiRenderFunc) {
		//we won't be using the drawcommand here, it's empty
		RecordFunc<VkCommandBuffer, Ref<DrawCommand>> recordFunc = [=](VkCommandBuffer commandBuffer, Ref<DrawCommand>) {
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
		imguiElement.RecordFunc = *(RecordFunc<void*, Ref<DrawCommand>>*) & recordFunc;

		s_CommandQueue->Enqueue(imguiElement);
	}
}