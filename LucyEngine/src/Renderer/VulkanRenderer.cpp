#include "lypch.h"
#include "VulkanRenderer.h"

#include "Context/RenderContext.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"

#include "VulkanRenderCommand.h"
#include "Mesh.h"

#include "Utils.h"

namespace Lucy {

	uint32_t VulkanRenderer::s_ImageIndex = 0;

	VulkanRenderer::VulkanRenderer(RenderArchitecture renderArchitecture)
		: RendererAPI(renderArchitecture) {
	}

	void VulkanRenderer::Init() {
		m_RenderContext = RenderContext::Create(m_Architecture);
		auto& vulkanRenderContext = As(m_RenderContext, VulkanContext);
		vulkanRenderContext->PrintInfo();

		m_RenderCommand = RenderCommand::Create();

		//RefLucy<Shader> pbrShader = Shader::Create("LucyPBR", "assets/shaders/LucyPBR.glsl");
		//RefLucy<Shader> idShader = Shader::Create("LucyID", "assets/shaders/LucyID.glsl");
		RefLucy<Shader> vulkanTestShader = Shader::Create("LucyVulkanTest", "assets/shaders/LucyVulkanTest.glsl");

		auto [width, height] = Utils::ReadSizeFromIni("Viewport");

		PipelineSpecification geometryPipelineSpecs;
		std::vector<ShaderLayoutElement> vertexLayout = {
				{ "a_Pos", ShaderDataSize::Float3 },
				{ "a_ID", ShaderDataSize::Float3 },
				{ "a_TextureCoords", ShaderDataSize::Float2 },
				{ "a_Normals", ShaderDataSize::Float3 },
				{ "a_Tangents", ShaderDataSize::Float3 },
				{ "a_BiTangents", ShaderDataSize::Float3 }
		};

		geometryPipelineSpecs.VertexShaderLayout = VertexShaderLayout(vertexLayout);
		geometryPipelineSpecs.Topology = Topology::TRIANGLES;
		geometryPipelineSpecs.Rasterization = { true, VK_CULL_MODE_BACK_BIT, 1.0f, PolygonMode::FILL };

		RenderPassSpecification geometryPassSpecs;
		geometryPassSpecs.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		RefLucy<VulkanRenderPass> geometryRenderPass = As(RenderPass::Create(geometryPassSpecs), VulkanRenderPass);

		FrameBufferSpecification geometryFrameBufferSpecs;
		geometryFrameBufferSpecs.ViewportWidth = width;
		geometryFrameBufferSpecs.ViewportHeight = height;
		geometryFrameBufferSpecs.RenderPass = geometryRenderPass;

		geometryPipelineSpecs.RenderPass = geometryRenderPass;
		geometryPipelineSpecs.FrameBuffer = FrameBuffer::Create(geometryFrameBufferSpecs);
		geometryPipelineSpecs.Shader = vulkanTestShader;

		m_GeometryPipeline = As(Pipeline::Create(geometryPipelineSpecs), VulkanPipeline);

		VkDevice device = VulkanDevice::Get().GetLogicalDevice();

		VkSemaphoreCreateInfo semaphoreCreateInfo{};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		m_ImageIsAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_RenderIsFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
		m_ImagesInFlight.resize(VulkanSwapChain::Get().GetImageCount(), VK_NULL_HANDLE);

		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			LUCY_VULKAN_ASSERT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &m_ImageIsAvailableSemaphores[i]));
			LUCY_VULKAN_ASSERT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &m_RenderIsFinishedSemaphores[i]));
			LUCY_VULKAN_ASSERT(vkCreateFence(device, &fenceCreateInfo, nullptr, &m_InFlightFences[i]));
		}
	}

	void VulkanRenderer::ClearCommands() {
		m_MeshDrawCommand.clear();
	}

	void VulkanRenderer::Draw() {
		auto& device = VulkanDevice::Get();
		VkDevice deviceVulkanHandle = device.GetLogicalDevice();

		//Wait for the frame to be finished
		vkWaitForFences(deviceVulkanHandle, 1, &m_InFlightFences[CURRENT_FRAME], VK_TRUE, UINT64_MAX);

		const auto& renderCommand = As(m_RenderCommand, VulkanRenderCommand);
		//recording to the command buffers
		renderCommand->Begin(m_GeometryPipeline);
		renderCommand->End(m_GeometryPipeline);

		VkSwapchainKHR swapChain = VulkanSwapChain::Get().GetVulkanHandle();

		vkAcquireNextImageKHR(deviceVulkanHandle, swapChain, UINT64_MAX, m_ImageIsAvailableSemaphores[CURRENT_FRAME], VK_NULL_HANDLE, &s_ImageIndex);

		if (m_ImagesInFlight[s_ImageIndex] != VK_NULL_HANDLE) {
			vkWaitForFences(deviceVulkanHandle, 1, &m_ImagesInFlight[s_ImageIndex], VK_TRUE, UINT64_MAX);
		}
		m_ImagesInFlight[s_ImageIndex] = m_InFlightFences[CURRENT_FRAME];
		auto a = m_InFlightFences[CURRENT_FRAME];

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore imageWaitSemaphores[] = { m_ImageIsAvailableSemaphores[CURRENT_FRAME] };
		VkPipelineStageFlags imageWaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = imageWaitSemaphores;
		submitInfo.pWaitDstStageMask = imageWaitStages;

		submitInfo.commandBufferCount = 1;
		VkCommandBuffer targetedCommandBuffer = renderCommand->GetCommandBuffer(s_ImageIndex);
		submitInfo.pCommandBuffers = &targetedCommandBuffer;

		VkSemaphore renderSignalSemaphores[] = { m_RenderIsFinishedSemaphores[CURRENT_FRAME] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = renderSignalSemaphores;

		vkResetFences(deviceVulkanHandle, 1, &m_InFlightFences[CURRENT_FRAME]);
		auto a1 = m_InFlightFences[CURRENT_FRAME];
		LUCY_VULKAN_ASSERT(vkQueueSubmit(device.GetGraphicsQueue(), 1, &submitInfo, m_InFlightFences[CURRENT_FRAME]));
	}

	void VulkanRenderer::Presentation() {
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &m_RenderIsFinishedSemaphores[CURRENT_FRAME];

		VkSwapchainKHR swapChainHandle = VulkanSwapChain::Get().GetVulkanHandle();
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapChainHandle;
		presentInfo.pImageIndices = &s_ImageIndex;
		presentInfo.pResults = nullptr;

		auto& device = VulkanDevice::Get();
		vkQueuePresentKHR(device.GetPresentQueue(), &presentInfo);
	
		CURRENT_FRAME = (CURRENT_FRAME + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void VulkanRenderer::Destroy() {
		VkDevice device = VulkanDevice::Get().GetLogicalDevice();
		vkDeviceWaitIdle(device);

		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(device, m_ImageIsAvailableSemaphores[i], nullptr);
			vkDestroySemaphore(device, m_RenderIsFinishedSemaphores[i], nullptr);
			vkDestroyFence(device, m_InFlightFences[i], nullptr);
		}
		As(m_RenderCommand, VulkanRenderCommand)->Destroy();

		m_GeometryPipeline->Destroy();
		m_RenderContext->Destroy();
	}

	void VulkanRenderer::Dispatch() {
		for (Func func : m_RenderQueue) {
			func();
		}
		m_RenderQueue.clear();
	}

	void VulkanRenderer::BeginScene(Scene& scene) {
		//EditorCamera& camera = scene.GetEditorCamera();
		//camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
		//camera.Update();

		m_ActiveScene = &scene;
	}

	void VulkanRenderer::EndScene() {
		Draw();
		Presentation();
	}

	void VulkanRenderer::Submit(const Func&& func) {
		m_RenderQueue.push_back(func);
	}

	void VulkanRenderer::SubmitMesh(RefLucy<Mesh> mesh, const glm::mat4& entityTransform) {
		Submit([this, mesh, entityTransform]() {
			m_MeshDrawCommand.push_back(MeshDrawCommand(mesh, entityTransform));
		});
	}

	void VulkanRenderer::OnFramebufferResize(float sizeX, float sizeY) {

	}

	Entity VulkanRenderer::OnMousePicking() {
		return {};
	}
}
