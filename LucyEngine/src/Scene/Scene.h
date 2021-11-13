#pragma once

#include "entt/entt.hpp"
#include "Camera.h"

namespace Lucy {

	class Entity;

	class Scene {
	public:
		Scene() = default;
		Scene(const Scene& other) = default;

		Entity CreateMesh(std::string& path);
		Entity CreateMesh();
		Entity CreateEntity();
		void RemoveEntity(Entity& e);

		inline EditorCamera& GetEditorCamera() { return m_Camera; }

		template <typename ... T>
		inline auto View() { return registry.view<T...>(); }
	private:
		entt::registry registry;
		EditorCamera m_Camera;

		friend class Entity;
	};
}

