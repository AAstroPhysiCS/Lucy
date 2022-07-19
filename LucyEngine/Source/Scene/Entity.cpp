#include "lypch.h"

#include "Entity.h"

namespace Lucy {

	Entity::Entity(Scene* scene, entt::entity& entity)
		: m_Scene(scene), m_Entity(entity) {
	}
}