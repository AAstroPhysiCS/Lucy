#pragma once

#include <iostream>

#include "glm/glm.hpp"

#include "UUID.h"
#include "../Renderer/Mesh.h"

namespace Lucy {

	struct TransformComponent {
		TransformComponent() = default;
		TransformComponent(glm::mat4& mat)
			: m_Mat(mat) {
		}
		TransformComponent(const TransformComponent& other) = default;

		inline glm::mat4& GetMatrix() { return m_Mat; }
		inline glm::vec3& GetPosition() { return m_Position; }
		inline glm::vec3& GetRotation() { return m_Rotation; }
		inline glm::vec3& GetScale() { return m_Scale; }

	private:
		glm::mat4 m_Mat = glm::mat4();
		glm::vec3 m_Position = glm::vec3();
		glm::vec3 m_Rotation = glm::vec3();
		glm::vec3 m_Scale = glm::vec3();
	};

	struct MeshComponent {
		MeshComponent() = default;
		MeshComponent(std::string& path)
			: m_Mesh(Mesh::Create(path)) {
		}
		MeshComponent(const MeshComponent& other) = default;

		void SetMesh(RefLucy<Mesh>&& mesh) { m_Mesh = std::move(mesh); }
		
		inline RefLucy<Mesh>& GetMesh() { return m_Mesh; }
		inline bool IsValid() { return m_Mesh.get() != nullptr && m_Mesh->GetSubmeshes().size() != 0; }
	private:
		RefLucy<Mesh> m_Mesh;
	};

	struct UUIDComponent {
		UUIDComponent() = default;
		UUIDComponent(UUID& uuid)
			: m_UUID(uuid) {
		}
		UUIDComponent(const UUIDComponent& other) = default;

		inline std::string& GetUUID() { return m_UUID.m_UUIDAsString; }

	private:
		UUID m_UUID;
	};

	struct TagComponent {
		TagComponent() = default;
		TagComponent(std::string& tag)
			: m_Tag(tag) {}
		TagComponent(const char* tag)
			: m_Tag(tag) {}
		TagComponent(const TagComponent& other) = default;

		inline std::string& GetTag() { return m_Tag; }
	private:
		std::string m_Tag = "Empty Entity";
	};
}