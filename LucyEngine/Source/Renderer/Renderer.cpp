#include "lypch.h"

#include "Renderer.h"

#include "Scene/Scene.h"
#include "Scene/Entity.h"

#include "Renderer/Descriptors/VulkanDescriptorSet.h"
#include "Renderer/VulkanRHI.h"
#include "Memory/Buffer/PushConstant.h"
#include "Memory/Buffer/Vulkan/VulkanFrameBuffer.h"
#include "Context/VulkanPipeline.h"
#include "Context/VulkanSwapChain.h"
#include "Context/VulkanDevice.h"

//#include "Context/OpenGLPipeline.h"

namespace Lucy {

	Ref<RHI> Renderer::s_RHI;

	RendererCreateInfo Renderer::s_CreateInfo;
	ShaderLibrary Renderer::s_ShaderLibrary;

	std::function<void(VkCommandBuffer commandBuffer)> Renderer::s_UIDrawDataFunc;

	void Renderer::Init(const RendererCreateInfo& rendererCreateInfo) {
		s_CreateInfo = rendererCreateInfo;
		s_RHI = RHI::Create(s_CreateInfo.Architecture);
		s_RHI->Init();

		auto& shaderLibrary = GetShaderLibrary();
		shaderLibrary.PushShader(Shader::Create("LucyPBR", "Assets/Shaders/LucyPBR.glsl"));
		shaderLibrary.PushShader(Shader::Create("LucyID", "Assets/Shaders/LucyID.glsl"));
	}

	void Renderer::Enqueue(const SubmitFunc&& func) {
		s_RHI->Enqueue(std::move(func));
	}

	void Renderer::SetUIDrawData(std::function<void(VkCommandBuffer commandBuffer)>&& func) {
		s_UIDrawDataFunc = func;
	}

	void Renderer::UIPass(const ImGuiPipeline& imguiPipeline) {
		if (s_CreateInfo.Architecture != RenderArchitecture::Vulkan)
			return;
		s_RHI.As<VulkanRHI>()->UIPass(imguiPipeline, std::move(Renderer::s_UIDrawDataFunc));
	}

	void Renderer::EnqueueStaticMesh(Priority priority, Ref<Mesh> mesh, const glm::mat4& entityTransform) {
		s_RHI->EnqueueStaticMesh(priority, mesh, entityTransform);
	}

	void Renderer::RecordStaticMeshToCommandQueue(Ref<Pipeline> pipeline, RecordFunc<Ref<DrawCommand>>&& func) {
		s_RHI->RecordStaticMeshToCommandQueue(pipeline, std::move(func));
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

	void Renderer::WaitForDevice() {
		//since it does not make any sense, for opengl to wait on "any device"
		if (s_CreateInfo.Architecture != RenderArchitecture::Vulkan)
			return;
		VkDevice device = VulkanDevice::Get().GetLogicalDevice();
		LUCY_VK_ASSERT(vkDeviceWaitIdle(device));
	}

	void Renderer::Destroy() {
		const ShaderLibrary& shaderLibrary = GetShaderLibrary();
		for (const Ref<Shader>& shader : shaderLibrary.m_Shaders)
			shader->Destroy();
		s_RHI->Destroy();
	}

	void Renderer::BindPipeline(Ref<Pipeline> pipeline) {
		PipelineBindInfo info;
		info.PipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		info.CommandBuffer = s_RHI->s_CommandQueue.GetCurrentCommandBuffer();

		pipeline->Bind(info);
	}

	void Renderer::BindDescriptorSet(Ref<Pipeline> pipeline, uint32_t descriptorSetIndex) {
		if (s_CreateInfo.Architecture == RenderArchitecture::Vulkan) {
			Ref<VulkanPipeline> vulkanPipeline = pipeline.As<VulkanPipeline>();

			VulkanDescriptorSetBindInfo bindInfo;
			bindInfo.CommandBuffer = s_RHI->s_CommandQueue.GetCurrentCommandBuffer();
			bindInfo.PipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			bindInfo.PipelineLayout = vulkanPipeline->GetPipelineLayout();

			auto& descriptorSet = vulkanPipeline->GetDescriptorSet<VulkanDescriptorSet>(descriptorSetIndex);
			descriptorSet->Update();
			descriptorSet->Bind(bindInfo);
			return;
		}
		//TODO: OpenGL
	}

	void Renderer::BeginRenderPass(Ref<Pipeline> pipeline, VkCommandBuffer commandBuffer) {
		Ref<RenderPass> renderPass = pipeline->GetRenderPass();
		Ref<FrameBuffer> frameBuffer = pipeline->GetFrameBuffer();

		RenderPassBeginInfo renderPassBeginInfo;
		renderPassBeginInfo.CommandBuffer = commandBuffer;
		renderPassBeginInfo.Width = frameBuffer->GetWidth();
		renderPassBeginInfo.Height = frameBuffer->GetHeight();

		if (s_CreateInfo.Architecture == RenderArchitecture::Vulkan) {
			VulkanSwapChain& swapChain = VulkanSwapChain::Get();
			renderPassBeginInfo.VulkanFrameBuffer = frameBuffer.As<VulkanFrameBuffer>()->GetVulkanHandles()[swapChain.GetCurrentFrameIndex()];
		}
		//TODO: OpenGL variant

		renderPass->Begin(renderPassBeginInfo);
	}

	void Renderer::EndRenderPass(Ref<Pipeline> pipeline) {
		Ref<RenderPass> renderPass = pipeline->GetRenderPass();
		renderPass->End();
	}

	void Renderer::BindBuffers(Ref<Mesh> mesh) {
		const auto& vertexBuffer = mesh->GetVertexBuffer();
		const auto& indexBuffer = mesh->GetIndexBuffer();
		//if (s_CreateInfo.Architecture == RenderArchitecture::OpenGL)
			//s_ActivePipeline.As<OpenGLPipeline>()->UploadVertexLayout(vertexBuffer);
		BindBuffers(vertexBuffer, indexBuffer);
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

	void Renderer::BindPushConstant(Ref<Pipeline> pipeline, const PushConstant& pushConstant) {
		PushConstantBindInfo bindInfo;
		bindInfo.CommandBuffer = s_RHI->s_CommandQueue.GetCurrentCommandBuffer();
		bindInfo.PipelineLayout = pipeline.As<VulkanPipeline>()->GetPipelineLayout();
		pushConstant.Bind(bindInfo);
	}

	//TODO: DELETE THIS
	void Renderer::UpdateResources(const std::vector<Ref<DrawCommand>>& drawCommands, Ref<Pipeline> pipeline) {
		for (Ref<MeshDrawCommand> cmd : drawCommands) {
			auto& materials = cmd->Mesh->GetMaterials();
			for (Submesh& submesh : cmd->Mesh->GetSubmeshes()) {
				const Ref<Material>& material = materials[submesh.MaterialIndex];
				//TODO: Only update it, when the material has changed
				material->Update(pipeline);
			}
		}
	}
}