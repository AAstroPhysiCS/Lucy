#include "lypch.h"
#include "VulkanRenderer.h"

#include "Context/RenderContext.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"

#include "Mesh.h"

#include "Utils.h"

namespace Lucy {

	uint32_t VulkanRenderer::s_ImageIndex = 0;
	RefLucy<VulkanCommandPool> VulkanRenderer::s_CommandPool;

	VulkanRenderer::VulkanRenderer(RenderArchitecture renderArchitecture)
		: RendererAPI(renderArchitecture) {
	}

	void VulkanRenderer::Init() {
		m_RenderContext = RenderContext::Create(m_Architecture);
		auto& vulkanRenderContext = As(m_RenderContext, VulkanContext);
		vulkanRenderContext->PrintInfo();

		//RefLucy<Shader> pbrShader = Shader::Create("LucyPBR", "assets/shaders/LucyPBR.glsl");
		//RefLucy<Shader> idShader = Shader::Create("LucyID", "assets/shaders/LucyID.glsl");
		RefLucy<Shader> vulkanTestShader = Shader::Create("LucyVulkanTest", "assets/shaders/LucyVulkanTest.glsl");

		PipelineSpecification geometryPipelineSpecs;
		std::vector<ShaderLayoutElement> vertexLayout = {
				{ "a_Pos", ShaderDataSize::Float2 },
				//{ "a_ID", ShaderDataSize::Float3 },
				//{ "a_TextureCoords", ShaderDataSize::Float2 },
				//{ "a_Normals", ShaderDataSize::Float3 },
				//{ "a_Tangents", ShaderDataSize::Float3 },
				//{ "a_BiTangents", ShaderDataSize::Float3 }
		};

		RenderPassSpecification geometryPassSpecs;
		geometryPassSpecs.ClearColor = { 0.0f, 0.0f, 0.5f, 1.0f };
		geometryPassSpecs.AttachmentReferences = { 
			{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }
		};
		
		FrameBufferSpecification geometryFrameBufferSpecs;
		geometryPipelineSpecs.VertexShaderLayout = VertexShaderLayout(vertexLayout);
		geometryPipelineSpecs.Topology = Topology::TRIANGLES;
		geometryPipelineSpecs.Rasterization = { true, VK_CULL_MODE_BACK_BIT, 1.0f, PolygonMode::FILL };
		geometryPipelineSpecs.RenderPass = RenderPass::Create(geometryPassSpecs);
		geometryPipelineSpecs.FrameBuffer = FrameBuffer::Create(geometryFrameBufferSpecs, geometryPipelineSpecs.RenderPass);
		geometryPipelineSpecs.Shader = vulkanTestShader;

		m_GeometryPipeline = As(Pipeline::Create(geometryPipelineSpecs), VulkanPipeline);

		m_ImageIsAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_RenderIsFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		uint32_t imageCount = VulkanSwapChain::Get().GetImageCount();
		m_ImagesInFlight.resize(imageCount, nullptr);

		s_CommandPool = VulkanCommandPool::Create({ VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, VK_COMMAND_BUFFER_LEVEL_PRIMARY, imageCount });
	}

	void VulkanRenderer::ClearCommands() {
		m_MeshDrawCommands.clear();
	}

	void VulkanRenderer::Execute() {
		auto& device = VulkanDevice::Get();
		VkDevice deviceVulkanHandle = device.GetLogicalDevice();

		VkFence currentFrameFence = m_InFlightFences[CURRENT_FRAME].GetFence();
		VkSemaphore currentFrameImageAvailSemaphore = m_ImageIsAvailableSemaphores[CURRENT_FRAME].GetSemaphore();
		VkSemaphore currentFrameRenderFinishedSemaphore = m_RenderIsFinishedSemaphores[CURRENT_FRAME].GetSemaphore();

		vkResetFences(deviceVulkanHandle, 1, &currentFrameFence);

		VkResult result = VulkanSwapChain::Get().AcquireNextImage(currentFrameImageAvailSemaphore, s_ImageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			return;
		}

		s_CommandPool->Execute(m_GeometryPipeline);

		if (m_ImagesInFlight[s_ImageIndex] != nullptr) {
			vkWaitForFences(deviceVulkanHandle, 1, &m_ImagesInFlight[s_ImageIndex]->GetFence(), VK_TRUE, UINT64_MAX);
		}
		m_ImagesInFlight[s_ImageIndex] = &m_InFlightFences[CURRENT_FRAME];

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore imageWaitSemaphores[] = { currentFrameImageAvailSemaphore };
		VkPipelineStageFlags imageWaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = imageWaitSemaphores;
		submitInfo.pWaitDstStageMask = imageWaitStages;

		VkCommandBuffer targetedCommandBuffer = s_CommandPool->GetCommandBuffer(s_ImageIndex);
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &targetedCommandBuffer;

		VkSemaphore renderSignalSemaphores[] = { currentFrameRenderFinishedSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = renderSignalSemaphores;

 		LUCY_VK_ASSERT(vkQueueSubmit(device.GetGraphicsQueue(), 1, &submitInfo, currentFrameFence));
		vkWaitForFences(deviceVulkanHandle, 1, &currentFrameFence, VK_TRUE, UINT64_MAX);
	}

	void VulkanRenderer::Present() {
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &m_RenderIsFinishedSemaphores[CURRENT_FRAME].GetSemaphore();

		VkSwapchainKHR swapChainHandle = VulkanSwapChain::Get().GetVulkanHandle();
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapChainHandle;
		presentInfo.pImageIndices = &s_ImageIndex;
		presentInfo.pResults = nullptr;

		auto& device = VulkanDevice::Get();
		VkResult result = vkQueuePresentKHR(device.GetPresentQueue(), &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
			OnFramebufferResize(0, 0);

		CURRENT_FRAME = (CURRENT_FRAME + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void VulkanRenderer::Destroy() {
		VkDevice device = VulkanDevice::Get().GetLogicalDevice();
		vkDeviceWaitIdle(device);

		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			m_ImageIsAvailableSemaphores[i].Destroy();
			m_RenderIsFinishedSemaphores[i].Destroy();
			m_InFlightFences[i].Destroy();
		}
		s_CommandPool->Destroy();

		m_GeometryPipeline->Destroy();
		m_RenderContext->Destroy();
	}

	void VulkanRenderer::Dispatch() {
		for (Func func : m_RenderFunctions) {
			func();
		}
		m_RenderFunctions.clear();
	}

	void VulkanRenderer::BeginScene(Scene& scene) {
		//EditorCamera& camera = scene.GetEditorCamera();
		//camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
		//camera.Update();

		m_ActiveScene = &scene;
	}

	void VulkanRenderer::EndScene() {
		Execute();
		Present();
	}

	// Should not be used in a loop
	void VulkanRenderer::DirectCopyBuffer(VkBuffer& stagingBuffer, VkBuffer& buffer, VkDeviceSize size) {
		s_CommandPool->DirectCopyBuffer(stagingBuffer, buffer, size);
	}

	void VulkanRenderer::Submit(const Func&& func) {
		m_RenderFunctions.push_back(func);
	}

	void VulkanRenderer::SubmitMesh(RefLucy<Mesh> mesh, const glm::mat4& entityTransform) {
		Submit([this, mesh, entityTransform]() {
			m_MeshDrawCommands.push_back(MeshDrawCommand(mesh, entityTransform));
		});
	}

	void VulkanRenderer::OnFramebufferResize(float sizeX, float sizeY) {
		vkDeviceWaitIdle(VulkanDevice::Get().GetLogicalDevice());
		vkResetFences(VulkanDevice::Get().GetLogicalDevice(), 1, &m_InFlightFences[CURRENT_FRAME].GetFence());

		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		swapChain.Recreate();

		s_CommandPool->Recreate();
		m_GeometryPipeline->Recreate(sizeX, sizeY);

		std::fill(m_ImagesInFlight.begin(), m_ImagesInFlight.end(), VK_NULL_HANDLE);
	}

	Entity VulkanRenderer::OnMousePicking() {
		return {};
	}
}
