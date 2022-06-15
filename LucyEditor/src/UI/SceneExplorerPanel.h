#pragma once

#include "Core/Panel.h"
#include "Scene/Entity.h"

namespace Lucy {

	class SceneExplorerPanel : public Panel {
	public:
		static SceneExplorerPanel& GetInstance();

		void OnEvent(Event& e) override;

		void SetEntityContext(Entity e);
		inline Entity& GetEntityContext() { return m_EntityContext; }

		void SetScene(Scene* scene);
		inline Scene* GetActiveScene() { return m_Scene; }
	private:
		SceneExplorerPanel() = default;
		virtual ~SceneExplorerPanel() = default;

		void Render();

		Entity m_EntityContext;
		Scene* m_Scene = nullptr;
	};
}