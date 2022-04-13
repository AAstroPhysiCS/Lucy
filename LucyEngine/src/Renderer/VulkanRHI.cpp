#include "lypch.h"
#include "VulkanRHI.h"

#include "Context/RenderContext.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "ViewportRenderer.h"

#include "Buffer/Vulkan/VulkanFrameBuffer.h"
#include "Renderer/VulkanAllocator.h"
#include "Utils.h"

namespace Lucy {

	VulkanRHI::VulkanRHI(RenderArchitecture renderArchitecture)
		: RHI(renderArchitecture) {
	}

	void VulkanRHI::Init() {
		m_RenderContext = RenderContext::Create(m_Architecture);
		auto& vulkanRenderContext = As(m_RenderContext, VulkanContext);
		vulkanRenderContext->PrintInfo();
		VulkanAllocator::Get().Init();
	}

	void VulkanRHI::ClearCommands() {
		m_RenderCommandQueue.Clear();
		m_MeshDrawCommands.clear();
	}

	void VulkanRHI::Dispatch() {
		for (const Func& func : m_RenderFunctions) {
			func();
		}
		m_RenderFunctions.clear();
	}

	void VulkanRHI::BeginScene(Scene& scene) {
		//EditorCamera& camera = scene.GetEditorCamera();
		//camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
		//camera.Update();
		m_ActiveScene = &scene;
	}

	void VulkanRHI::SubmitRenderCommand(const RenderCommand& renderCommand) {
		m_RenderCommandQueue.SubmitToQueue(renderCommand);
	}

	PresentResult VulkanRHI::RenderScene() {
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();

		swapChain.BeginFrame();
		swapChain.Execute(m_RenderCommandQueue);
		swapChain.EndFrame();

		return (PresentResult)swapChain.Present();
	}

	void VulkanRHI::EndScene() {
		//TODO: profiling
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

	void VulkanRHI::Submit(const Func&& func) {
		m_RenderFunctions.push_back(func);
	}

	void VulkanRHI::SubmitMesh(RefLucy<Pipeline> pipeline, RefLucy<Mesh> mesh, const glm::mat4& entityTransform) {
		Submit([=]() {
			m_MeshDrawCommands.push_back(MeshDrawCommand(pipeline, mesh, entityTransform));
		});
	}

	void VulkanRHI::RecordSingleTimeCommand(std::function<void(VkCommandBuffer)>&& func) {
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		VkCommandBuffer commandBuffer = swapChain.BeginSingleTimeCommand();
		func(commandBuffer);
		swapChain.EndSingleTimeCommand(commandBuffer);
	}

	void VulkanRHI::Destroy() {
		VkDevice device = VulkanDevice::Get().GetLogicalDevice();
		vkDeviceWaitIdle(device);
		m_RenderContext->Destroy();
	}

	void VulkanRHI::OnViewportResize() {
	}

	Entity VulkanRHI::OnMousePicking() {
		return {};
	}
}
