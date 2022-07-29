#pragma once

#include "entt/entt.hpp"
#include "Camera.h"

namespace Lucy {

	class Entity;

	class Scene {
	public:
		Scene() = default;
		~Scene() = default;

		Entity CreateMesh(std::string& path);
		Entity CreateMesh();
		Entity CreateEntity();
		void RemoveEntity(Entity& e);
		Entity GetEntityByMeshID(const glm::vec3& meshID);

		inline EditorCamera& GetEditorCamera() { return m_Camera; }

		void Update(float viewportWidth, float viewportHeight);
		void Destroy();

		template <typename ... T>
		inline auto View() { return m_Registry.view<T...>(); }
	private:
		entt::registry m_Registry;
		EditorCamera m_Camera;

		friend class Entity;
	};
}

