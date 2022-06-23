#include "lypch.h"

#include "Scene.h"
#include "Components.h"

#include "Renderer/Renderer.h"
#include "Renderer/Context/Pipeline.h"

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
		entt::entity entity = m_Registry.create();
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

	Entity Scene::GetEntityByPixelValue(const glm::vec3& pixelValue) {
		auto view = m_Registry.view<MeshComponent>();
		for (auto entity : view) {
			Entity e{ this, entity };
			MeshComponent& meshComponent = e.GetComponent<MeshComponent>();
			const Ref<Mesh>& mesh = meshComponent.GetMesh();
			const glm::vec3& meshPixelValue = mesh->GetMeshPixelValue() / 255.0f;

			if (glm::round(meshPixelValue * 10e4f) / 10e4f == pixelValue) 
				return e;
		}
		LUCY_ASSERT(false);
	}
	
	void Scene::Update() {
		auto& [sizeX, sizeY] = Renderer::GetViewportSize();

		EditorCamera& camera = GetEditorCamera();
		camera.SetViewportSize(sizeX, sizeY);
		camera.Update();

		const auto& meshView = View<MeshComponent>();
		for (auto entity : meshView) {
			Entity e{ this, entity };
			MeshComponent& meshComponent = e.GetComponent<MeshComponent>();
			if (!meshComponent.IsValid()) continue;

			Renderer::EnqueueStaticMesh(meshComponent.GetMesh(), e.GetComponent<TransformComponent>().GetMatrix());
		}
	}
}