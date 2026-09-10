#pragma once

#include "entt/entt.hpp"
#include "Camera.h"

#include <ranges>

namespace Lucy {

	class Entity;
	struct Event;

	template <typename TComponent>
	concept IsComponent = requires(TComponent&& component) {
		{ component.IsValid() } -> std::same_as<bool>;
	};

	class Scene final {
	public:
		Scene() = default;
		~Scene() = default;

		Scene(const Scene&) = delete;
		Scene& operator=(const Scene&) = delete;
		Scene(Scene&&) = delete;
		Scene& operator=(Scene&&) = delete;

		Entity CreateMesh(std::string& path);
		Entity CreateMesh();
		Entity CreateEntity();
		void RemoveEntity(Entity& e);
		Entity GetEntityByMeshID(uint32_t meshID);

		void SetEntityContext(Entity e);
		Entity GetEntityContext();

		EditorCamera& GetEditorCamera() { return m_Camera; }

		void OnEvent(Event& e);
		void Update(float deltaTime);
		void Destroy();

		template <typename ... TComponents>
		inline decltype(auto) View() { return m_Registry.view<TComponents...>(); }

		template<typename ... TComponents>
		inline bool CheckForComponentValidity(TComponents&& ... components) const {
			bool success = true;
			((success &= components.IsValid()) && ...);
			return success;
		}

		template <IsComponent ... TComponents, typename TFunc>
		inline void ViewRForEach(TFunc func) {
			auto view = m_Registry.view<TComponents...>();
			for (auto entity : view | std::views::reverse) {
				auto allComponents = view.get(entity);
				std::apply([&](TComponents&... components) {
					if (!CheckForComponentValidity(components...))
						return;
					func(components...);
				}, allComponents);
			}
		}

		template <IsComponent ... TComponents, typename TFunc>
		inline void ViewForEach(TFunc func) {
			auto view = m_Registry.view<TComponents...>();
			for (auto entity : view) {
				auto allComponents = view.get(entity);
				std::apply([&](TComponents&... components) {
					if (!CheckForComponentValidity(components...))
						return;
					func(components...);
				}, allComponents);
			}
		}
	private:
		void UpdateCamera(int32_t viewportWidth, int32_t viewportHeight);

		entt::registry m_Registry;
		EditorCamera m_Camera { 0.01f, 1000.0f, 90.0f };
		entt::entity m_EntityContext = static_cast<entt::entity>(std::numeric_limits<uint32_t>::max());

		friend class Entity;
	};
}