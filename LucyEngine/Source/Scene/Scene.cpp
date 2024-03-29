#include "lypch.h"

#include "Entity.h"
#include "Scene.h"
#include "Components.h"

#include "Renderer/Renderer.h"

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
		const entt::entity entity = m_Registry.create();
		Entity e{ this, entity };

		e.AddComponent<UUIDComponent>();
		e.AddComponent<TransformComponent>();
		e.AddComponent<TagComponent>();
		return e;
	}

	void Scene::RemoveEntity(Entity& e) {
		//TODO: free all resources depending on the entity
		m_Registry.destroy(e.m_Entity);
	}

	Entity Scene::GetEntityByMeshID(const glm::vec3& meshID) {
		auto view = m_Registry.view<MeshComponent>();
		for (auto entity : view) {
			Entity e{ this, entity };
			MeshComponent& meshComponent = e.GetComponent<MeshComponent>();
			const Ref<Mesh>& mesh = meshComponent.GetMesh();
			const glm::vec3& meshIDValue = mesh->GetMeshID();

			if (meshIDValue == meshID)
				return e;
		}
		LUCY_ASSERT(false, "Getting an entity by ID failed!");
		return {};
	}

	void Scene::Update(int32_t viewportWidth, int32_t viewportHeight) {
		LUCY_PROFILE_NEW_EVENT("Scene::Update");

		m_Camera.SetViewportSize(viewportWidth, viewportHeight);
		m_Camera.Update();
	}

	void Scene::Destroy() {
		const auto& meshView = View<MeshComponent>();
		for (auto entity : meshView) {
			Entity e{ this, entity };
			MeshComponent meshComponent = e.GetComponent<MeshComponent>();
			if (!meshComponent.IsValid()) 
				continue;

			meshComponent.GetMesh()->Destroy();
		}

		const auto& cubemapView = View<HDRCubemapComponent>();
		for (auto entity : cubemapView) {
			Entity e{ this, entity };
			HDRCubemapComponent cubemapComponent = e.GetComponent<HDRCubemapComponent>();
			if (!cubemapComponent.IsValid()) 
				continue;

			cubemapComponent.GetCubemapImage()->Destroy();
		}
	}
}