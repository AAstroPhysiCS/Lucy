#include "EditorLayer.h"

namespace Lucy {

	EditorLayer* EditorLayer::s_Instance = nullptr;

	void EditorLayer::Begin()
	{
		auto& rendererAPI = Renderer::GetRendererAPI();

		rendererAPI->ClearColor(0.0f, 0.0f, 0.0f, 1.0f); //delete after a separate framebuffer initialization
		rendererAPI->Clear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	}

	void EditorLayer::End()
	{
		//later
	}

	void EditorLayer::OnRender()
	{
		//later
	}

	void EditorLayer::OnEvent()
	{
		//later
	}

	void EditorLayer::Destroy()
	{
		delete s_Instance;
	}

}