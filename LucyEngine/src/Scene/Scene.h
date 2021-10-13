#pragma once

#include <vector>

#include "entt/entt.hpp"

namespace Lucy {

	class Entity;

	class Scene
	{
	public:

		Scene() = default;

		Entity CreateEntity(std::string& path);
		Entity CreateEntity();
		void RemoveEntity(Entity& e);

	private:
		entt::registry registry;

		friend class Entity;
		friend class SceneHierarchyPanel;
		friend class EditorLayer;
	};
}

