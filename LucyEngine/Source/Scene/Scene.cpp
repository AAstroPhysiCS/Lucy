#include "lypch.h"

#include "Entity.h"
#include "Scene.h"
#include "Components.h"
#include "Events/EventHandler.h"

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
			if (!mesh)
				continue;
			const glm::vec3& meshIDValue = mesh->GetMeshID();

			if (meshIDValue == meshID)
				return e;
		}
		LUCY_ASSERT(false, "Getting an entity by ID failed!");
		return {};
	}

	void Scene::Update() {
		LUCY_PROFILE_NEW_EVENT("Scene::Update");
		m_Camera.Update();
	}

	void Scene::UpdateCamera(int32_t viewportWidth, int32_t viewportHeight) {
		m_Camera.SetAspectRatio((float)viewportWidth / viewportHeight);
		m_Camera.Update();
	}

	void Scene::OnEvent(Event& e) {
		EventHandler::AddListener<ViewportAreaResizeEvent>(e, [this](const ViewportAreaResizeEvent& evt) {
			UpdateCamera(evt.GetWidth(), evt.GetHeight());
		});

		EventHandler::AddListener<CursorPosEvent>(e, [this](const CursorPosEvent& evt) {
			m_Camera.Update();
		});
	}

	void Scene::Destroy() {
		ViewForEach<MeshComponent>([](MeshComponent& meshComponent) {
			meshComponent.GetMesh()->Destroy();
		});

		ViewForEach<HDRCubemapComponent>([&](HDRCubemapComponent& cubemapComponent) {
			cubemapComponent.Destroy();
		});
	}
}