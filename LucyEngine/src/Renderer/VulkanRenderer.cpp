#include "lypch.h"
#include "VulkanRenderer.h"

#include "Context/RenderContext.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"

#include "Mesh.h"

namespace Lucy {

	VulkanRenderer::VulkanRenderer(RenderArchitecture renderArchitecture)
		: RendererAPI(renderArchitecture) {
	}

	void VulkanRenderer::Init() {
		m_RenderContext = RenderContext::Create(m_Architecture);
		m_RenderContext->PrintInfo();
	}

	void VulkanRenderer::ClearCommands() {

	}

	void VulkanRenderer::Draw() {

	}

	void VulkanRenderer::Destroy() {
		m_RenderContext->Destroy();
	}

	void VulkanRenderer::Dispatch() {

	}

	void VulkanRenderer::BeginScene(Scene& scene) {

	}

	void VulkanRenderer::EndScene() {

	}

	void VulkanRenderer::Submit(const Func&& func) {

	}

	void VulkanRenderer::SubmitMesh(RefLucy<Mesh> mesh, const glm::mat4& entityTransform) {

	}

	void VulkanRenderer::OnFramebufferResize(float sizeX, float sizeY) {

	}

	Entity VulkanRenderer::OnMousePicking() {
		return {};
	}
}
