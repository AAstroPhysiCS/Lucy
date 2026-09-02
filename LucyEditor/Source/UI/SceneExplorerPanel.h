#pragma once

#include "Core/Panel.h"
#include "Scene/Entity.h"

namespace Lucy {

	class SceneExplorerPanel : public Panel {
	public:
		static SceneExplorerPanel& GetInstance();

		void OnEvent(Event& e) final override;

		Entity GetEntityContext() { return m_Scene->GetEntityContext(); }

		void SetScene(Ref<Scene> scene);
		inline Ref<Scene> GetActiveScene() { return m_Scene; }
	private:
		SceneExplorerPanel() = default;
		virtual ~SceneExplorerPanel() = default;

		void Render();

		Ref<Scene> m_Scene = nullptr;
	};
}