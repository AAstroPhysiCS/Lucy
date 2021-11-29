#pragma once

#include "UI/Panel.h"
#include "Scene/Entity.h"

namespace Lucy {

	class SceneHierarchyPanel : public Panel
	{
	public:
		static SceneHierarchyPanel& GetInstance();

		void SetEntityContext(Entity e);
		inline Entity& GetEntityContext() { return m_EntityContext; }
	private:
		void Render();

		Entity m_EntityContext;
	};
}

