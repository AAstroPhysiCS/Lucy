#include "lypch.h"

#include "Renderer.h"
#include "Context/RHI.h"

#include "Buffer/UniformBuffer.h"

#include "Shader/Shader.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"

#include "Renderer/VulkanRHI.h"

namespace Lucy {

	RefLucy<Window> Renderer::s_Window;
	RefLucy<RHI> Renderer::s_RendererAPI;

	RenderArchitecture Renderer::s_SelectedArchitecture;

	void Renderer::Init(RefLucy<Window> window, RenderArchitecture renderArchitecture) {
		s_Window = window;
		s_SelectedArchitecture = renderArchitecture;

		s_RendererAPI = RHI::Create(renderArchitecture);
		s_RendererAPI->Init();

		auto& shaderLibrary = GetShaderLibrary();
		shaderLibrary.PushShader(Shader::Create("LucyPBR", "assets/shaders/LucyPBR.glsl"));
		shaderLibrary.PushShader(Shader::Create("LucyID", "assets/shaders/LucyID.glsl"));
		shaderLibrary.PushShader(Shader::Create("LucyVulkanTest", "assets/shaders/LucyVulkanTest.glsl"));
	}

	void Renderer::Submit(const Func&& func) {
		s_RendererAPI->Submit(std::move(func));
	}

	void Renderer::SubmitUIPass(const std::function<void(VkCommandBuffer commandBuffer)>&& func) {
		s_UIPassFunc = func;
	}

	void Renderer::SubmitMesh(RefLucy<Pipeline> pipeline, RefLucy<Mesh> mesh, const glm::mat4& entityTransform) {
		s_RendererAPI->SubmitMesh(pipeline, mesh, entityTransform);
	}

	void Renderer::SubmitRenderCommand(const RenderCommand& renderCommand) {
		s_RendererAPI->SubmitRenderCommand(renderCommand);
	}

	void Renderer::OnViewportResize() {
		s_RendererAPI->OnViewportResize();
	}

	Entity Renderer::OnMousePicking() {
		return s_RendererAPI->OnMousePicking();
	}

	void Renderer::Dispatch() {
		s_RendererAPI->Dispatch();
	}

	void Renderer::ClearDrawCommands() {
		s_RendererAPI->ClearCommands();
	}

	void Renderer::SetViewportSize(int32_t width, int32_t height) {
		s_RendererAPI->SetViewportSize(width, height);
	}

	void Renderer::SetViewportMouse(float viewportMouseX, float viewportMouseY) {
		s_RendererAPI->SetViewportMouse(viewportMouseX, viewportMouseY);
	}

	void Renderer::BeginScene(Scene& scene) {
		s_Window->Update();
		s_RendererAPI->BeginScene(scene);
	}

	void Renderer::EndScene() {
		s_RendererAPI->EndScene();
		glfwSwapBuffers(s_Window->Raw());
	}

	PresentResult Renderer::RenderScene() {
		return s_RendererAPI->RenderScene();
	}

	void Renderer::Destroy() {
		const ShaderLibrary& shaderLibrary = GetShaderLibrary();
		for (const RefLucy<Shader> shader : shaderLibrary.m_Shaders)
			shader->Destroy();
		s_RendererAPI->Destroy();
	}

	void Renderer::SetViewportMousePosition(float x, float y) {
		s_RendererAPI->SetViewportMousePosition(x, y);
	}
}