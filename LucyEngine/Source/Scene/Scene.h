#pragma once

#include "entt/entt.hpp"
#include "Camera.h"

#include <ranges>

namespace Lucy {

	class Entity;

	template <typename TComponent>
	concept IsComponent = requires(TComponent&& component) {
		{ component.IsValid() } -> std::same_as<bool>;
	};

	class Scene final {
	public:
		Scene() = default;
		~Scene() = default;

		Entity CreateMesh(std::string& path);
		Entity CreateMesh();
		Entity CreateEntity();
		void RemoveEntity(Entity& e);
		Entity GetEntityByMeshID(const glm::vec3& meshID);

		inline EditorCamera& GetEditorCamera() { return m_Camera; }

		void OnEvent(Event& e);
		void Update();
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
		EditorCamera m_Camera { 0.25f, 250.0f, 90.0f };

		friend class Entity;
	};
}