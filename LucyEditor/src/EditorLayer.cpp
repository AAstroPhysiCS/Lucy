#include "EditorLayer.h"

namespace Lucy {

	EditorLayer* EditorLayer::s_Instance = nullptr;

	void EditorLayer::Update()
	{

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f); //needs to change bcs we dont want the default fb
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		//rendererAPI->ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		//rendererAPI->Clear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	}

	void EditorLayer::OnEvent()
	{
	}

	void EditorLayer::Destroy()
	{
		delete s_Instance;
	}

}