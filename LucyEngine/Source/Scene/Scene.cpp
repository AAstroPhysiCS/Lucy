#include "lypch.h"

#include "Entity.h"
#include "Scene.h"
#include "Components.h"

#include "SceneImporter.h"

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

	CameraComponent* Scene::GetPrimaryCamera() {
		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view) {
			CameraComponent& camera = view.get<CameraComponent>(entity);
			if (camera.IsPrimary())
				return &camera;
		}
		return nullptr;
	}

	bool Scene::LoadScene(const std::string& path, const std::string& primaryCamera) {
		SceneImporter importer;
		if (!importer.Import(path)) {
			LUCY_CRITICAL("Scene could not be imported!");
			LUCY_CRITICAL(importer.GetError());
			return false;
		}

		ImportedScene& importedScene = importer.GetScene();

		if (!importedScene.Meshes.empty()) {
			Entity meshEntity = CreateEntity();
			meshEntity.GetComponent<TagComponent>().SetTag(importedScene.Name);
			meshEntity.AddComponent<MeshComponent>(importedScene, path);

			Renderer::EnqueueToRenderCommandQueue([meshEntity](const Ref<RenderDevice>& device) mutable {
				MeshComponent& meshComponent = meshEntity.GetComponent<MeshComponent>();

				const auto& mesh = meshComponent.GetMesh();
				const auto& transform = meshEntity.GetComponent<TransformComponent>().GetMatrix();
				meshComponent.SetObjectHandle(device->GetScene()->RTRegisterObject(mesh->GetRenderDeviceMeshHandle(), transform, RenderDeviceObjectFlags::None));
			});
		}

		for (const ImportedCamera& importedCamera : importedScene.Cameras) {
			Entity cameraEntity = CreateEntity();
			cameraEntity.GetComponent<TagComponent>().SetTag(importedCamera.Name);
			
			//overrides the transformcomponent
			cameraEntity.AddComponent<TransformComponent>(glm::translate(glm::mat4{ 1.0f }, importedCamera.Position) * glm::mat4_cast(importedCamera.Orientation));

			CameraComponent& cameraComponent = cameraEntity.AddComponent<CameraComponent>(importedCamera);
			if (!primaryCamera.empty() && importedCamera.Name == primaryCamera)
				cameraComponent.SetPrimary(true);
		}

		for (const ImportedLight& importedLight : importedScene.Lights) {
			Entity lightEntity = CreateEntity();
			lightEntity.GetComponent<TagComponent>().SetTag(importedLight.Name);

			switch (importedLight.Type) {
				case ImportedLightType::Directional: {
					lightEntity.AddComponent<DirectionalLightComponent>(importedLight.Direction, importedLight.Color);
					break;
				}
				case ImportedLightType::Point: {
					glm::mat4 transform = glm::translate(glm::mat4{ 1.0f }, importedLight.Position);
					lightEntity.AddComponent<TransformComponent>(transform);
					lightEntity.AddComponent<PunctualLightComponent>(importedLight);
					break;
				}
				case ImportedLightType::Spot: {
					glm::quat orientation = glm::quatLookAtRH(glm::normalize(importedLight.Direction), glm::normalize(importedLight.Up));
					glm::mat4 transform = glm::translate(glm::mat4{ 1.0f }, importedLight.Position) * glm::mat4_cast(orientation);

					lightEntity.AddComponent<TransformComponent>(transform);
					lightEntity.AddComponent<PunctualLightComponent>(importedLight);
					break;
				}
				default:
					LUCY_ASSERT(false, "Unsupported light type: {0}", static_cast<int32_t>(importedLight.Type));
					break;
			}
		}

		LUCY_INFO("Imported scene '{}': {} meshes, {} cameras, {} lights, {} animations", importedScene.Name, importedScene.Meshes.size(), 
			importedScene.Cameras.size(), importedScene.Lights.size(), importedScene.Animations.size());

		return true;
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
		float aspectRatio = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);
		m_Camera.SetAspectRatio(aspectRatio);
		ViewForEach<CameraComponent>([aspectRatio](CameraComponent& cameraComponent) {
			auto& camera = cameraComponent.GetCamera();
			camera.SetAspectRatio(aspectRatio);
			camera.UpdateProjection();
		});
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