#include "lypch.h"

#include "Renderer.h"
#include "Context/RHI.h"

#include "Memory/Buffer/UniformBuffer.h"

#include "Shader/Shader.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"

#include "ViewportRenderer.h"
#include "Memory/Buffer/Vulkan/VulkanFrameBuffer.h"
#include "Renderer/VulkanRHI.h"

#include "Context/OpenGLPipeline.h"

namespace Lucy {

	Ref<RHI> Renderer::s_RHI;
	Ref<Pipeline> Renderer::s_ActivePipeline;

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

	void Renderer::UIPass(const ImGuiPipeline& imguiPipeline) {
		if (s_Specs.Architecture != RenderArchitecture::Vulkan)
			return;

		//a hack (TODO: Change this)
		Renderer::RecordToCommandQueue([=]() {
			VkCommandBuffer commandBuffer = s_RHI->s_CommandQueue.GetCurrentCommandBuffer();
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
			Renderer::s_UIDrawDataFunc(commandBuffer);
			renderPass->End();
		});
	}

	void Renderer::EnqueueStaticMesh(Ref<Mesh> mesh, const glm::mat4& entityTransform) {
		s_RHI->EnqueueStaticMesh(mesh, entityTransform);
	}

	void Renderer::RecordToCommandQueue(RecordFunc<>&& func) {
		s_RHI->RecordToCommandQueue(std::move(func));
	}

	void Renderer::RecordToCommandQueue(RecordFunc<MeshDrawCommand>&& func) {
		s_RHI->RecordToCommandQueue(std::move(func));
	}

	void Renderer::OnWindowResize() {
		s_RHI->OnWindowResize();
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
		scene.Update();
		s_RHI->BeginScene(scene);
	}

	void Renderer::RenderScene() {
		s_RHI->RenderScene();
	}

	PresentResult Renderer::EndScene() {
		return s_RHI->EndScene();
	}

	void Renderer::Destroy() {
		const ShaderLibrary& shaderLibrary = GetShaderLibrary();
		for (const Ref<Shader> shader : shaderLibrary.m_Shaders)
			shader->Destroy();
		s_RHI->Destroy();
	}

	void Renderer::BindPipeline(Ref<Pipeline> pipeline) {
		s_ActivePipeline = pipeline;
		s_RHI->BindPipeline(pipeline);
	}

	void Renderer::UnbindPipeline() {
		s_RHI->UnbindPipeline(s_ActivePipeline);
	}

	void Renderer::BindBuffers(Ref<Mesh> mesh) {
		auto& vertexBuffer = mesh->GetVertexBuffer();
		auto& indexBuffer = mesh->GetIndexBuffer();
		if (s_Specs.Architecture == RenderArchitecture::OpenGL)
			s_ActivePipeline.As<OpenGLPipeline>()->UploadVertexLayout(vertexBuffer);
		BindBuffers(vertexBuffer, mesh->GetIndexBuffer());
	}

	void Renderer::BindBuffers(Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer) {
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