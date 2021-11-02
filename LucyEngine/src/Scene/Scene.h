#pragma once

#include <vector>

#include "entt/entt.hpp"

namespace Lucy {

	class Entity;

	class Scene
	{
	public:
		Scene() = default;
		Scene(const Scene& other) = default;
		//Scene(Scene&& other) = default;

		Entity CreateMesh(std::string& path);
		Entity CreateMesh();
		Entity CreateEntity();
		void RemoveEntity(Entity& e);

		template <typename ... T>
		inline auto View() { return registry.view<T...>(); }
	private:
		entt::registry registry;

		friend class Entity;
		friend class EditorLayer;
	};
}

