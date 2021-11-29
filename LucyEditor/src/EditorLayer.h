#pragma once

#include "Core/Layer.h"
#include "Core/Base.h"
#include "Core/Window.h"
#include "Scene/Scene.h"

#include "Events/EventDispatcher.h"
#include "Events/InputEvent.h"

namespace Lucy {

	class EditorLayer : public Layer {
	public:
		static EditorLayer& GetInstance() {
			static EditorLayer s_Instance;
			return s_Instance;
		}

		void Begin(PerformanceMetrics& rendererMetrics);
		void End();
		void Init(RefLucy<Window> window);
		void OnRender();
		void OnEvent(Event& e);
		void Destroy();

		inline Scene& GetScene() { return m_Scene; }
	private:
		EditorLayer() = default;
		virtual ~EditorLayer() = default;

		RefLucy<Window> m_Window;
		Scene m_Scene;
	};
}