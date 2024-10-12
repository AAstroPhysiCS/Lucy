#pragma once

#include "Core/Application.h"
#include "EditorOverlay.h"

using namespace Lucy;

class EditorApplication : public Application {
public:
	EditorApplication(const ApplicationArgs& args, const ApplicationCreateInfo& applicationCreateInfo)
		: Application(args, applicationCreateInfo) {
		Ref<Scene> defaultScene = Memory::CreateRef<Scene>();
		Ref<Overlay> editorOverlay = Memory::CreateRef<EditorOverlay>(defaultScene);
		//defaultScene->AddEntity("Grid"); //e.g.
		SetScene(defaultScene);
		SetRenderType(RenderType::Rasterizer);
		PushOverlay(editorOverlay);
	}

	virtual ~EditorApplication() = default;
};

namespace Lucy {

	Application* CreateApplication(const ApplicationArgs& args, const ApplicationCreateInfo& applicationCreateInfo) {
		return new EditorApplication(args, applicationCreateInfo);
	}
}