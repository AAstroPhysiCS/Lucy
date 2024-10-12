#pragma once

#include "Core/Base.h"
#include "Components.h"
#include "Scene.h"

#include "entt/entt.hpp"

namespace Lucy {

	class Entity {
	public:
		Entity() = default;
		Entity(Scene* scene, entt::entity entity)
			: m_Entity(entity), m_Scene(scene) {
		}
		~Entity() = default;

		inline bool operator==(Entity& other) {
			return GetComponent<UUIDComponent>().GetUUID() == other.GetComponent<UUIDComponent>().GetUUID();
		}

		template <typename ... TComponent>
		inline bool HasComponent() {
			LUCY_ASSERT(IsValid());
			return m_Scene->m_Registry.all_of<TComponent...>(m_Entity);
		}

		inline bool IsValid() {
			if ((ENTT_ID_TYPE)m_Entity == std::numeric_limits<ENTT_ID_TYPE>::max()) return false;
			return m_Scene->m_Registry.valid(m_Entity);
		}

		template <typename ... Args>
		void function(Args&& ... args) {

		}

		template <typename TComponent, typename ... Args>
		inline TComponent& AddComponent(Args&& ... args) {
			if (HasComponent<TComponent>()) return m_Scene->m_Registry.replace<TComponent>(m_Entity, std::forward<Args>(args)...);
			return m_Scene->m_Registry.emplace<TComponent>(m_Entity, std::forward<Args>(args)...);
		}

		template <typename TComponent>
		inline TComponent& GetComponent() {
			LUCY_ASSERT(IsValid());
			return m_Scene->m_Registry.get<TComponent>(m_Entity);
		}
	private:
		entt::entity m_Entity = (entt::entity) std::numeric_limits<uint32_t>::max();
		Scene* m_Scene = nullptr;

		friend class Scene;
	};
}