#pragma once

#include "../Core/Base.h"
#include "Components.h"
#include "Scene.h"

#include "entt/entt.hpp"

namespace Lucy {

	class Entity {
	public:
		Entity() = default;
		Entity(Scene* scene, entt::entity& entity);

		bool operator==(Entity& other) {
			return GetComponent<UUIDComponent>().GetUUID() == other.GetComponent<UUIDComponent>().GetUUID();
		}

		template <typename ... T>
		bool HasComponent() {
			LUCY_ASSERT(IsValid());
			return m_Scene->m_Registry.all_of<T...>(m_Entity);
		}

		bool IsValid() {
			if ((ENTT_ID_TYPE)m_Entity == std::numeric_limits<ENTT_ID_TYPE>::max()) return false;
			return m_Scene->m_Registry.valid(m_Entity);
		}

		template <typename T, typename ... Args>
		T& AddComponent(Args&& ... args) {
			if (HasComponent<T>()) return m_Scene->m_Registry.replace<T>(m_Entity, std::forward<Args>(args)...);
			return m_Scene->m_Registry.emplace<T>(m_Entity, std::forward<Args>(args)...);
		}

		template <typename T>
		T& GetComponent() {
			LUCY_ASSERT(IsValid());
			return m_Scene->m_Registry.get<T>(m_Entity);
		}

	private:
		entt::entity m_Entity = (entt::entity) std::numeric_limits<uint32_t>::max();
		Scene* m_Scene = nullptr;

		friend class Scene;
	};
}