#include "lypch.h"
#include "VulkanRHI.h"

#include "Context/RenderContext.h"
#include "Context/VulkanPipeline.h"

#include "Scene/Scene.h"
#include "Scene/Entity.h"

#include "ViewportRenderer.h"
#include "Renderer/Renderer.h"

#include "Buffer/Vulkan/VulkanFrameBuffer.h"
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

	void VulkanRHI::EnqueueStaticMesh(RefLucy<Mesh> mesh, const glm::mat4& entityTransform) {
		Enqueue([=]() {
			m_StaticMeshDrawCommandQueue.push_back(MeshDrawCommand(mesh, entityTransform));
		});
	}

	void VulkanRHI::RecordToCommandQueue(RecordFunc<MeshDrawCommand>&& func) {
		//TODO: Temporary, delete afterwards
		CommandElement element;
		element.RecordFunc = *(RecordFunc<void*>*) & func;
		element.Argument = nullptr;
		s_CommandQueue.Enqueue(element);
		/*
		for (MeshDrawCommand cmd : m_StaticMeshDrawCommandQueue) {
			CommandElement element;
			element.RecordFunc = *(RecordFunc<void*>*)&func;
			element.Argument = (void*)&cmd;
			s_CommandQueue.Enqueue(element);
		}
		*/
	}

	void VulkanRHI::BindPipeline(RefLucy<Pipeline> pipeline) {
		PipelineBindInfo info;
		info.PipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		info.CommandBuffer = s_CommandQueue.GetCurrentCommandBuffer();
		pipeline->Bind(info);
	}

	void VulkanRHI::UnbindPipeline(RefLucy<Pipeline> pipeline) {
		pipeline->Unbind();
	}

	void VulkanRHI::BindBuffers(RefLucy<VertexBuffer> vertexBuffer, RefLucy<IndexBuffer> indexBuffer) {
		VertexBindInfo vertexInfo;
		vertexInfo.CommandBuffer = s_CommandQueue.GetCurrentCommandBuffer();
		vertexBuffer->Bind(vertexInfo);

		IndexBindInfo indexInfo;
		indexInfo.CommandBuffer = vertexInfo.CommandBuffer;
		indexBuffer->Bind(indexInfo);
	}

	void VulkanRHI::RecordSingleTimeCommand(std::function<void(VkCommandBuffer)>&& func) {
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		VkCommandBuffer commandBuffer = s_CommandQueue.BeginSingleTimeCommand();
		func(commandBuffer);
		s_CommandQueue.EndSingleTimeCommand(commandBuffer);
	}

	void VulkanRHI::Destroy() {
		VkDevice device = VulkanDevice::Get().GetLogicalDevice();
		vkDeviceWaitIdle(device);
		s_CommandQueue.Free();
		m_RenderContext->Destroy();
	}

	void VulkanRHI::OnViewportResize() {
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		swapChain.Recreate();

		As(ViewportRenderer::s_GeometryPipeline, VulkanPipeline)->Recreate();

		//Possible (there is also an error)
		Renderer::Enqueue([]() {
			ViewportRenderer::s_ImGuiPipeline.UIFramebuffer->Destroy();
			ViewportRenderer::s_ImGuiPipeline.UIRenderPass->Recreate();
		});

		auto& desc = swapChain.GetSwapChainFrameBufferDesc();
		desc->RenderPass = ViewportRenderer::s_ImGuiPipeline.UIRenderPass;

		FrameBufferSpecification frameBufferSpecs;
		frameBufferSpecs.Width = swapChain.GetExtent().width;
		frameBufferSpecs.Height = swapChain.GetExtent().height;
		frameBufferSpecs.InternalInfo = desc;

		Renderer::Enqueue([=]() mutable {
			ViewportRenderer::s_ImGuiPipeline.UIFramebuffer = FrameBuffer::Create(frameBufferSpecs);
		});

		auto& extent = swapChain.GetExtent();
		SetViewportSize(extent.width, extent.height);

		//s_CommandQueue.Recreate();
	}

	Entity VulkanRHI::OnMousePicking() {
		return {};
	}
}
