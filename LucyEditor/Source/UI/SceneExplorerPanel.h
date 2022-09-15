#pragma once

#include "Core/Panel.h"
#include "Scene/Entity.h"

namespace Lucy {

	class SceneExplorerPanel : public Panel {
	public:
		static SceneExplorerPanel& GetInstance();

		void OnEvent(Event& e) final override;

		void SetEntityContext(Entity e);
		inline Entity& GetEntityContext() { return m_EntityContext; }

		void SetIDPipeline(Ref<GraphicsPipeline> pipeline);

		void SetScene(Ref<Scene> scene);
		inline Ref<Scene> GetActiveScene() { return m_Scene; }
	private:
		SceneExplorerPanel() = default;
		virtual ~SceneExplorerPanel() = default;

		void Render();

		Entity m_EntityContext;
		Ref<Scene> m_Scene = nullptr;

		Ref<GraphicsPipeline> m_IDPipeline = nullptr;
	};
}