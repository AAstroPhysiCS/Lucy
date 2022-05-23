#include "lypch.h"

#include "Renderer.h"
#include "Context/RHI.h"

#include "Buffer/UniformBuffer.h"

#include "Shader/Shader.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"

#include "ViewportRenderer.h"
#include "Buffer/Vulkan/VulkanFrameBuffer.h"
#include "Renderer/VulkanRHI.h"

#include "Context/OpenGLPipeline.h"

namespace Lucy {

	RefLucy<RHI> Renderer::s_RHI;
	RefLucy<Pipeline> Renderer::s_ActivePipeline;

	RendererSpecification Renderer::s_Specs;
	ShaderLibrary Renderer::s_ShaderLibrary;

	std::function<void(VkCommandBuffer commandBuffer)> Renderer::s_UIDrawDataFunc;

	void Renderer::Init(const RendererSpecification& specs) {
		s_Specs = specs;
		s_RHI = RHI::Create(s_Specs.Architecture);
		s_RHI->Init();

		auto& shaderLibrary = GetShaderLibrary();
		shaderLibrary.PushShader(Shader::Create("LucyPBR", "assets/shaders/LucyPBR.glsl"));
		shaderLibrary.PushShader(Shader::Create("LucyID", "assets/shaders/LucyID.glsl"));
		shaderLibrary.PushShader(Shader::Create("LucyVulkanTest", "assets/shaders/LucyVulkanTest.glsl"));
	}

	void Renderer::Enqueue(const SubmitFunc&& func) {
		s_RHI->Enqueue(std::move(func));
	}

	void Renderer::SetUIDrawData(std::function<void(VkCommandBuffer commandBuffer)>&& func) {
		s_UIDrawDataFunc = func;
	}

	void Renderer::EnqueueStaticMesh(RefLucy<Mesh> mesh, const glm::mat4& entityTransform) {
		s_RHI->EnqueueStaticMesh(mesh, entityTransform);
	}

	void Renderer::RecordToCommandQueue(RecordFunc<MeshDrawCommand>&& func) {
		s_RHI->RecordToCommandQueue(std::move(func));
	}

	void Renderer::RenderUI(const ImGuiPipeline& imGuiPipeline) {
		if (s_Specs.Architecture != RenderArchitecture::Vulkan || !s_UIDrawDataFunc) 
			return;

		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		auto& frameBufferHandle = As(imGuiPipeline.UIFramebuffer, VulkanFrameBuffer);
		auto& renderPass = As(imGuiPipeline.UIRenderPass, VulkanRenderPass);

		VkCommandBuffer commandBuffer = s_RHI->s_CommandQueue.GetCurrentCommandBuffer();
		const auto& targetFrameBuffer = frameBufferHandle->GetVulkanHandles()[swapChain.GetCurrentImageIndex()];

		VkCommandBufferBeginInfo cmdBeginInfo{};
		cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		LUCY_VK_ASSERT(vkBeginCommandBuffer(commandBuffer, &cmdBeginInfo));

		VkViewport viewport;
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = swapChain.GetExtent().width;
		viewport.height = swapChain.GetExtent().height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swapChain.GetExtent();

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		RenderPassBeginInfo beginInfo;
		beginInfo.CommandBuffer = commandBuffer;
		beginInfo.VulkanFrameBuffer = targetFrameBuffer;
		renderPass->Begin(beginInfo);

		s_UIDrawDataFunc(commandBuffer);

		renderPass->End();
		LUCY_VK_ASSERT(vkEndCommandBuffer(commandBuffer));
	}

	void Renderer::OnViewportResize() {
		s_RHI->OnViewportResize();
	}

	Entity Renderer::OnMousePicking() {
		return s_RHI->OnMousePicking();
	}

	void Renderer::Dispatch() {
		s_RHI->Dispatch();
	}

	void Renderer::ClearQueues() {
		s_RHI->ClearQueues();
	}

	void Renderer::SetViewportSize(int32_t width, int32_t height) {
		s_RHI->SetViewportSize(width, height);
	}

	void Renderer::SetViewportMouse(float viewportMouseX, float viewportMouseY) {
		s_RHI->SetViewportMouse(viewportMouseX, viewportMouseY);
	}

	void Renderer::BeginScene(Scene& scene) {
		s_Specs.Window->Update();
		scene.Update();
		s_RHI->BeginScene(scene);
	}

	void Renderer::RenderScene() {
		s_RHI->RenderScene();
	}

	PresentResult Renderer::EndScene() {
		PresentResult result = s_RHI->EndScene();
		glfwSwapBuffers(s_Specs.Window->Raw());
		return result;
	}

	void Renderer::Destroy() {
		const ShaderLibrary& shaderLibrary = GetShaderLibrary();
		for (const RefLucy<Shader> shader : shaderLibrary.m_Shaders)
			shader->Destroy();
		s_RHI->Destroy();
		s_Specs.Window->Destroy();
	}

	void Renderer::SetViewportMousePosition(float x, float y) {
		s_RHI->SetViewportMousePosition(x, y);
	}

	void Renderer::BindPipeline(RefLucy<Pipeline> pipeline) {
		s_ActivePipeline = pipeline;
		s_RHI->BindPipeline(pipeline);
	}

	void Renderer::UnbindPipeline() {
		s_RHI->UnbindPipeline(s_ActivePipeline);
	}

	void Renderer::BindBuffers(RefLucy<Mesh> mesh) {
		auto& vertexBuffer = mesh->GetVertexBuffer();
		auto& indexBuffer = mesh->GetIndexBuffer();
		if (s_Specs.Architecture == RenderArchitecture::OpenGL)
			As(s_ActivePipeline, OpenGLPipeline)->UploadVertexLayout(vertexBuffer);
		BindBuffers(vertexBuffer, mesh->GetIndexBuffer());
	}

	void Renderer::BindBuffers(RefLucy<VertexBuffer> vertexBuffer, RefLucy<IndexBuffer> indexBuffer) {
		s_RHI->BindBuffers(vertexBuffer, indexBuffer);
	}

	void Renderer::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
							   int32_t vertexOffset, uint32_t firstInstance) {
		if (Renderer::GetCurrentRenderArchitecture() == RenderArchitecture::Vulkan) {
			VkCommandBuffer commandBuffer = s_RHI->s_CommandQueue.GetCurrentCommandBuffer();
			vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
		}
	}
}