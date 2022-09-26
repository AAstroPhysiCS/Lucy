#include "lypch.h"
#include "Renderer.h"
#include "Renderer/VulkanRenderer.h"

#include "Shader/ShaderLibrary.h"

#include "Scene/Entity.h"

#include "Utils/Utils.h"

namespace Lucy {

	void Renderer::Init(RenderArchitecture arch, Ref<Window>& window) {
		s_Architecture = arch;
		
		auto [viewportWidth, viewportHeight] = Utils::ReadAttributeFromIni("Viewport", "Size");
		SetViewportArea(viewportWidth, viewportHeight);

		s_Renderer = RendererBase::Create(arch, window);
		s_Renderer->Init();

		ShaderLibrary::Get().Init();
	}

	void Renderer::BeginScene(Ref<Scene>& scene) {
		LUCY_PROFILE_NEW_EVENT("Renderer::BeginScene");

		const auto& [viewportWidth, viewportHeight] = Renderer::GetViewportArea();
		scene->Update(viewportWidth, viewportHeight);

		s_Renderer->BeginScene(scene);

		s_Scene = scene;
	}

	void Renderer::RenderScene() {
		LUCY_PROFILE_NEW_EVENT("Renderer::RenderScene");
		s_Renderer->RenderScene();
	}

	RenderContextResultCodes Renderer::EndScene() {
		LUCY_PROFILE_NEW_EVENT("Renderer::EndScene");
		return s_Renderer->EndScene();
	}

	void Renderer::WaitForDevice() {
		LUCY_PROFILE_NEW_EVENT("Renderer::WaitForDevice");
		return s_Renderer->WaitForDevice();
	}

	void Renderer::Destroy() {
		return s_Renderer->Destroy();
	}

	void Renderer::BindBuffers(void* commandBufferHandle, Ref<Mesh> mesh) {
		LUCY_PROFILE_NEW_EVENT("Renderer::BindBuffers");
		s_Renderer->GetRenderDevice()->BindBuffers(commandBufferHandle, mesh);
	}

	void Renderer::BindPushConstant(void* commandBufferHandle, Ref<GraphicsPipeline> pipeline, const VulkanPushConstant& pushConstant) {
		LUCY_PROFILE_NEW_EVENT("Renderer::BindPushConstant");
		s_Renderer->GetRenderDevice()->BindPushConstant(commandBufferHandle, pipeline, pushConstant);
	}

	void Renderer::BindPipeline(void* commandBufferHandle, Ref<GraphicsPipeline> pipeline) {
		LUCY_PROFILE_NEW_EVENT("Renderer::BindPipeline");
		s_Renderer->GetRenderDevice()->BindPipeline(commandBufferHandle, pipeline);
	}

	void Renderer::BindAllDescriptorSets(void* commandBufferHandle, Ref<GraphicsPipeline> pipeline) {
		LUCY_PROFILE_NEW_EVENT("Renderer::BindAllDescriptorSets");
		s_Renderer->GetRenderDevice()->BindAllDescriptorSets(commandBufferHandle, pipeline);
	}

	void Renderer::UpdateDescriptorSets(Ref<GraphicsPipeline> pipeline) {
		LUCY_PROFILE_NEW_EVENT("Renderer::UpdateDescriptorSets");
		s_Renderer->GetRenderDevice()->UpdateDescriptorSets(pipeline);
	}

	void Renderer::BindDescriptorSet(void* commandBufferHandle, Ref<GraphicsPipeline> pipeline, uint32_t setIndex) {
		LUCY_PROFILE_NEW_EVENT("Renderer::BindDescriptorSet");
		s_Renderer->GetRenderDevice()->BindDescriptorSet(commandBufferHandle, pipeline, setIndex);
	}

	void Renderer::BindBuffers(void* commandBufferHandle, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer) {
		LUCY_PROFILE_NEW_EVENT("Renderer::BindBuffers");
		s_Renderer->GetRenderDevice()->BindBuffers(commandBufferHandle, vertexBuffer, indexBuffer);
	}

	void Renderer::DrawIndexed(void* commandBufferHandle, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
		LUCY_PROFILE_NEW_EVENT("Renderer::DrawIndexed");
		s_Renderer->GetRenderDevice()->DrawIndexed(commandBufferHandle, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void Renderer::BeginRenderPass(void* commandBufferHandle, Ref<GraphicsPipeline> pipeline) {
		LUCY_PROFILE_NEW_EVENT("Renderer::BeginRenderPass");
		s_Renderer->GetRenderDevice()->BeginRenderPass(commandBufferHandle, pipeline);
	}

	void Renderer::EndRenderPass(Ref<GraphicsPipeline> pipeline) {
		LUCY_PROFILE_NEW_EVENT("Renderer::EndRenderPass");
		s_Renderer->GetRenderDevice()->EndRenderPass(pipeline);
	}

	void Renderer::DirectCopyBuffer(VkBuffer& stagingBuffer, VkBuffer& buffer, VkDeviceSize size) {
		s_Renderer.As<VulkanRenderer>()->DirectCopyBuffer(stagingBuffer, buffer, size);
	}

	void Renderer::SubmitImmediateCommand(std::function<void(VkCommandBuffer)>&& func) {
		s_Renderer.As<VulkanRenderer>()->SubmitImmediateCommand(std::move(func));
	}

	CommandResourceHandle Renderer::CreateCommandResource(Ref<GraphicsPipeline> pipeline, CommandFunc&& func) {
		LUCY_PROFILE_NEW_EVENT("Renderer::CreateCommandResource");
		return s_Renderer->GetRenderDevice()->CreateCommandResource(pipeline, std::move(func));
	}

	CommandResourceHandle Renderer::CreateChildCommandResource(CommandResourceHandle parentResourceHandle, Ref<GraphicsPipeline> childPipeline, CommandFunc&& func) {
		LUCY_PROFILE_NEW_EVENT("Renderer::CreateChildCommandResource");
		return s_Renderer->GetRenderDevice()->CreateChildCommandResource(parentResourceHandle, childPipeline, std::move(func));
	}

	void Renderer::EnqueueCommandResourceFree(CommandResourceHandle resourceHandle) {
		LUCY_PROFILE_NEW_EVENT("Renderer::EnqueueCommandResourceFree");
		return s_Renderer->GetRenderDevice()->EnqueueCommandResourceFree(resourceHandle);
	}
	
	void Renderer::EnqueueToRenderThread(EnqueueFunc&& func) {
		LUCY_PROFILE_NEW_EVENT("Renderer::EnqueueToRenderThread");
		s_Renderer->GetRenderDevice()->EnqueueToRenderThread(std::forward<EnqueueFunc>(func));
	}

	void Renderer::OnWindowResize() {
		LUCY_PROFILE_NEW_EVENT("Renderer::OnWindowResize");
		s_Renderer->OnWindowResize();
	}

	void Renderer::OnViewportResize() {
		LUCY_PROFILE_NEW_EVENT("Renderer::OnViewportResize");
		s_Renderer->OnViewportResize();
	}

	Entity Renderer::OnMousePicking(const Ref<GraphicsPipeline>& idPipeline) {
		LUCY_PROFILE_NEW_EVENT("Renderer::OnMousePicking");
		return s_Renderer->OnMousePicking(s_Scene, idPipeline);
	}

	void Renderer::SetViewportMouse(float viewportMouseX, float viewportMouseY) {
		s_ViewportMouseX = viewportMouseX;
		s_ViewportMouseY = viewportMouseY;
	}

	void Renderer::SetImGuiRenderData(std::function<void(VkCommandBuffer)>&& func) {
		s_ImGuiRenderData = func;
	}

	void Renderer::SetViewportArea(int32_t width, int32_t height) {
		s_ViewportWidth = width;
		s_ViewportHeight = height;
	}
}