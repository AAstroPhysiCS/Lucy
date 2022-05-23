#pragma once

#include "Core/Module.h"
#include "Core/Base.h"
#include "Core/Window.h"
#include "Scene/Scene.h"

#include "Renderer/ViewportRenderer.h"

#include "Events/EventDispatcher.h"
#include "Events/InputEvent.h"

namespace Lucy {

	class EditorModule : public Module {
	public:
		static EditorModule& GetInstance() {
			static EditorModule s_Instance;
			return s_Instance;
		}

		void Init(RefLucy<Window> window) override;
		void Begin(PerformanceMetrics& rendererMetrics) override;
		void End() override;
		void OnRender() override;
		void OnEvent(Event& e) override;
		void Destroy() override;

		inline Scene& GetScene() { return m_Scene; }
	private:
		EditorModule() = default;
		virtual ~EditorModule() = default;

		Scene m_Scene;
		ViewportRenderer m_ViewportRenderer;
	};
}