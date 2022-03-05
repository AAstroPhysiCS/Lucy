#include "lypch.h"

#include "Scene.h"
#include "Components.h"

#include "Entity.h"

namespace Lucy {

	Entity Scene::CreateMesh(std::string& path) {
		Entity e = CreateEntity();
		e.AddComponent<MeshComponent>(path);
		return e;
	}

	Entity Scene::CreateMesh() {
		Entity e = CreateEntity();
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

	Entity Scene::GetEntityByPixelValue(const glm::vec3& pixelValue) {
		auto view = registry.view<MeshComponent>();
		for (auto entity : view) {
			Entity e{ this, entity };
			MeshComponent& meshComponent = e.GetComponent<MeshComponent>();
			const RefLucy<Mesh>& mesh = meshComponent.GetMesh();
			const glm::vec3& meshPixelValue = mesh->GetMeshPixelValue() / 255.0f;

			if (glm::round(meshPixelValue * 10e4f) / 10e4f == pixelValue) 
				return e;
		}
		LUCY_ASSERT(false);
	}
}