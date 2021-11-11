#include "Scene.h"
#include "Components.h"

#include "Entity.h"

namespace Lucy {

	Entity Scene::CreateMesh(std::string& path) {
		Entity& e = CreateEntity();
		e.AddComponent<MeshComponent>(path);
		return e;
	}

	Entity Scene::CreateMesh() {
		Entity& e = CreateEntity();
		e.AddComponent<TagComponent>("Empty Mesh");
		e.AddComponent<MeshComponent>();
		return e;
	}

	Entity Scene::CreateEntity() {
		entt::entity entity = registry.create();
		Entity e{ this, entity };

		e.AddComponent<UUIDComponent>();
		e.AddComponent<TransformComponent>();
		e.AddComponent<TagComponent>();
		return e;
	}

	void Scene::RemoveEntity(Entity& e) {
		//TODO: free all resources depending on the entity
		registry.destroy(e.m_Entity);
	}
}