#pragma once

#include "Core/Layer.h"
#include "Core/Base.h"
#include "Core/Window.h"
#include "Scene/Scene.h"

#include "Renderer/ViewportRenderer.h"

#include "Events/EventDispatcher.h"
#include "Events/InputEvent.h"

namespace Lucy {

	class EditorLayer : public Layer {
	public:
		static EditorLayer& GetInstance() {
			static EditorLayer s_Instance;
			return s_Instance;
		}

		void Init(RefLucy<Window> window);
		void Begin(PerformanceMetrics& rendererMetrics) override;
		void End() override;
		void OnRender() override;
		void OnEvent(Event& e) override;
		void Destroy() override;

		inline Scene& GetScene() { return m_Scene; }
	private:
		EditorLayer() = default;
		virtual ~EditorLayer() = default;

		RefLucy<Window> m_Window;
		Scene m_Scene;
		ViewportRenderer m_ViewportRenderer;
	};
}