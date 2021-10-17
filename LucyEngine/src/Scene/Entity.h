#pragma once


#include "Components.h"
#include "Scene.h"

#include "entt/entt.hpp"

namespace Lucy {

	class Entity
	{

	private:
		Entity(Scene* scene, entt::entity& entity);

		entt::entity m_Entity;
		Scene* m_Scene;
	public:
		bool operator==(Entity& other) {
			return GetComponent<UUIDComponent>().GetUUID() == other.GetComponent<UUIDComponent>().GetUUID();
		}

		template <typename T, typename ... Args>
		T& AddComponent(Args&& ... args) {
			return m_Scene->registry.emplace<T>(m_Entity, std::forward<Args>(args)...);
		}

		template <typename T>
		T& GetComponent() {
			return m_Scene->registry.get<T>(m_Entity);
		}

		friend class Scene;
	};
}