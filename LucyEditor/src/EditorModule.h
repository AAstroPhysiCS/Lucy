#pragma once

#include "Core/Module.h"
#include "Core/Base.h"
#include "Core/Window.h"
#include "Scene/Scene.h"

#include "Renderer/ViewportRenderer.h"

#include "Events/EventDispatcher.h"
#include "Events/InputEvent.h"

#include "ImGuiOverlay.h"

namespace Lucy {

	class EditorModule : public Module {
	public:
		EditorModule(Ref<Window> window);
		virtual ~EditorModule() = default;
		
		void Begin() override;
		void End() override;
		void OnRender() override;
		void OnEvent(Event& e) override;
		void Destroy() override;

		inline Scene& GetScene() { return m_Scene; }
	private:
		Scene m_Scene;
		ViewportRenderer m_ViewportRenderer;
		ImGuiOverlay m_ImGuiOverlay;
	};
}