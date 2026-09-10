#include "lypch.h"

#include "Entity.h"
#include "Scene.h"
#include "Components.h"

#include "Renderer/Device/RenderDeviceScene.h"

#include "Events/EventHandler.h"

namespace Lucy {
	
	Entity Scene::CreateMesh(std::string& path) {
		Entity e = CreateEntity();
		e.AddComponent<MeshComponent>(path);

		Renderer::EnqueueToRenderCommandQueue([this, e](const Ref<RenderDevice>& device) mutable {
			auto& component = e.GetComponent<MeshComponent>();
			const auto& mesh = component.GetMesh();
			const auto& transform = e.GetComponent<TransformComponent>().GetMatrix();
			auto objectHandle = device->GetScene()->RTRegisterObject(mesh->GetRenderDeviceMeshHandle(), transform, RenderDeviceObjectFlags::None);
			component.SetObjectHandle(objectHandle);
		});

		return e;
	}

	void Scene::SetEntityContext(Entity e) {
		m_EntityContext = e.m_Entity;
	}
	
	Entity Scene::GetEntityContext() {
		return Entity{ this, m_EntityContext };
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

	Entity Scene::GetEntityByMeshID(uint32_t meshID) {
		auto view = m_Registry.view<MeshComponent>();
		for (auto entity : view) {
			Entity e{ this, entity };
			MeshComponent& meshComponent = e.GetComponent<MeshComponent>();
			const Ref<Mesh>& mesh = meshComponent.GetMesh();
			if (!mesh)
				continue;
			uint32_t meshIDValue = meshComponent.GetRenderDeviceObjectHandle().Index + 1;

			if (meshIDValue == meshID)
				return e;
		}
		LUCY_ASSERT(false, "Getting an entity by ID failed!");
		return {};
	}

	void Scene::Update(float deltaTime) {
		LUCY_PROFILE_NEW_EVENT("Scene::Update");
		m_Camera.Update(deltaTime);
	}

	void Scene::UpdateCamera(int32_t viewportWidth, int32_t viewportHeight) {
		m_Camera.SetAspectRatio((float)viewportWidth / viewportHeight);
	}

	void Scene::OnEvent(Event& e) {
		EventHandler::AddListener<ViewportAreaResizeEvent>(e, [this](const ViewportAreaResizeEvent& evt) {
			UpdateCamera(evt.GetWidth(), evt.GetHeight());
		});
	}

	void Scene::Destroy() {
		ViewForEach<MeshComponent>([](MeshComponent& meshComponent) {
			meshComponent.GetMesh()->Destroy();
		});
	}
}