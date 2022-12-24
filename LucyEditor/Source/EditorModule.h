#pragma once

#include "Core/Module.h"
#include "Core/Base.h"
#include "Core/Window.h"
#include "Scene/Scene.h"

#include "ImGuiOverlay.h"

namespace Lucy {

	class Window;
	class RendererModule;

	class EditorModule : public Module {
	public:
		EditorModule(Ref<Window> window, Ref<Scene> scene, Ref<RendererModule> rendererModule);
		virtual ~EditorModule() = default;
		
		void Begin() final override;
		void End() final override;
		void OnRender() final override;
		void OnEvent(Event& e) final override;
		void Destroy() final override;
		void Wait() final override;
	private:
		ImGuiOverlay m_ImGuiOverlay;
	};
}