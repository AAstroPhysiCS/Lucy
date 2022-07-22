#include "lypch.h"

#include "Renderer.h"

#include "Scene/Scene.h"
#include "Scene/Entity.h"

#include "VulkanRenderDevice.h"

namespace Lucy {

	Ref<RenderDevice> Renderer::s_RenderDevice = nullptr;
	Ref<Window> Renderer::s_Window = nullptr;
	RenderArchitecture Renderer::s_Arch;

	ShaderLibrary Renderer::s_ShaderLibrary;

	std::function<void(VkCommandBuffer commandBuffer)> Renderer::s_UIDrawDataFunc;

	void Renderer::Init(RenderArchitecture arch, Ref<Window> window) {
		s_Arch = arch;
		s_Window = window;

		s_RenderDevice = RenderDevice::Create(arch);
		s_RenderDevice->Init();

		auto& shaderLibrary = GetShaderLibrary();
		shaderLibrary.PushShader(Shader::Create("LucyPBR", "Assets/Shaders/LucyPBR.glsl"));
		shaderLibrary.PushShader(Shader::Create("LucyID", "Assets/Shaders/LucyID.glsl"));
	}

	void Renderer::WaitForDevice() {
		s_RenderDevice->Wait();
	}

	void Renderer::Enqueue(EnqueueFunc&& func) {
		s_RenderDevice->Enqueue(std::move(func));
	}

	void Renderer::EnqueueStaticMesh(Priority priority, Ref<Mesh> mesh, const glm::mat4& entityTransform) {
		s_RenderDevice->EnqueueStaticMesh(priority, mesh, entityTransform);
	}

	void Renderer::RecordStaticMeshToCommandQueue(Ref<Pipeline> pipeline, RecordFunc<VkCommandBuffer, Ref<DrawCommand>>&& func) {
		s_RenderDevice->RecordStaticMeshToCommandQueue(pipeline, std::move(func));
	}

	void Renderer::OnWindowResize() {
		s_RenderDevice->OnWindowResize();
	}

	void Renderer::OnViewportResize() {
		s_RenderDevice->OnViewportResize();
	}

	Entity Renderer::OnMousePicking() {
		return s_RenderDevice->OnMousePicking();
	}

	void Renderer::Dispatch() {
		s_RenderDevice->Dispatch();
	}

	void Renderer::ClearQueues() {
		s_RenderDevice->ClearQueues();
	}

	void Renderer::SetViewportSize(int32_t width, int32_t height) {
		s_RenderDevice->SetViewportSize(width, height);
	}

	void Renderer::SetViewportMouse(float viewportMouseX, float viewportMouseY) {
		s_RenderDevice->SetViewportMouse(viewportMouseX, viewportMouseY);
	}

	void Renderer::BeginScene(Scene& scene) {
		scene.Update();
		s_RenderDevice->BeginScene(scene);
	}

	void Renderer::RenderScene() {
		s_RenderDevice->RenderScene();
	}

	PresentResult Renderer::EndScene() {
		return s_RenderDevice->EndScene();
	}

	void Renderer::Destroy() {
		const ShaderLibrary& shaderLibrary = GetShaderLibrary();
		for (const Ref<Shader>& shader : shaderLibrary.m_Shaders)
			shader->Destroy();
		s_RenderDevice->Destroy();
	}

	void Renderer::BindPipeline(VkCommandBuffer commandBuffer, Ref<Pipeline> pipeline) {
		s_RenderDevice->BindPipeline(commandBuffer, pipeline);
	}

	void Renderer::BindDescriptorSet(void* commandBufferHandle, Ref<Pipeline> pipeline, Ref<DescriptorSet> descriptorSet) {
		s_RenderDevice->BindDescriptorSet(commandBufferHandle, pipeline, descriptorSet);
	}

	void Renderer::BeginRenderPass(VkCommandBuffer commandBuffer, Ref<Pipeline> pipeline) {
		s_RenderDevice->BeginRenderPass(commandBuffer, pipeline);
	}

	void Renderer::EndRenderPass(Ref<Pipeline> pipeline) {
		s_RenderDevice->EndRenderPass(pipeline);
	}

	void Renderer::BindBuffers(VkCommandBuffer commandBuffer, Ref<Mesh> mesh) {
		s_RenderDevice->BindBuffers(commandBuffer, mesh->GetVertexBuffer(), mesh->GetIndexBuffer());
	}

	void Renderer::DrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount,
							   uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
		s_RenderDevice->DrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void Renderer::BindPushConstant(VkCommandBuffer commandBuffer, Ref<Pipeline> pipeline, const PushConstant& pushConstant) {
		s_RenderDevice->BindPushConstant(commandBuffer, pipeline, pushConstant);
	}

	void Renderer::UpdateResources(const std::vector<Ref<DrawCommand>>& drawCommands, Ref<Pipeline> pipeline) {
		for (Ref<MeshDrawCommand> cmd : drawCommands) {
			auto& materials = cmd->Mesh->GetMaterials();
			for (Submesh& submesh : cmd->Mesh->GetSubmeshes()) {
				const Ref<Material>& material = materials[submesh.MaterialIndex];
				material->Update(pipeline);
			}
		}
	}

	void Renderer::SetUIDrawData(std::function<void(VkCommandBuffer commandBuffer)>&& func) {
		s_UIDrawDataFunc = func;
	}

	void Renderer::UIPass(const ImGuiPipeline& imguiPipeline) {
		if (s_Arch != RenderArchitecture::Vulkan)
			return;
		s_RenderDevice.As<VulkanRenderDevice>()->UIPass(imguiPipeline, std::move(Renderer::s_UIDrawDataFunc));
	}
}