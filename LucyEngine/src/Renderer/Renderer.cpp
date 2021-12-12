#include "lypch.h"

#include "Renderer.h"
#include "Context/RendererAPI.h"

#include "Buffer/UniformBuffer.h"

#include "Shader/Shader.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"

namespace Lucy {

	RefLucy<Window> Renderer::s_Window;
	RefLucy<RendererAPI> Renderer::s_RendererAPI;

	RenderArchitecture Renderer::s_SelectedArchitecture;

	void Renderer::Init(RefLucy<Window> window, RenderArchitecture renderArchitecture) {
		s_Window = window;
		s_SelectedArchitecture = renderArchitecture;

		s_RendererAPI = RendererAPI::Create(renderArchitecture);
		s_RendererAPI->Init();
	}

	void Renderer::Submit(const Func&& func) {
		s_RendererAPI->Submit(std::move(func));
	}

	void Renderer::SubmitMesh(RefLucy<Mesh> mesh, const glm::mat4& entityTransform) {
		s_RendererAPI->SubmitMesh(mesh, entityTransform);
	}

	void Renderer::OnFramebufferResize(float sizeX, float sizeY) {
		s_RendererAPI->OnFramebufferResize(sizeX, sizeY);
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

	void Renderer::BeginScene(Scene& scene) {
		s_Window->Update();
		s_RendererAPI->BeginScene(scene);
	}

	void Renderer::EndScene() {
		s_RendererAPI->EndScene();
		glfwSwapBuffers(s_Window->Raw());
	}

	void Renderer::Destroy() {
		s_RendererAPI->Destroy();
	}

	void Renderer::SetViewportMousePosition(float x, float y) {
		s_RendererAPI->SetViewportMousePosition(x, y);
	}
}