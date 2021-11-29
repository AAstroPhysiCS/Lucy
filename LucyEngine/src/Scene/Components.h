#pragma once

#include "glm/glm.hpp"
#include "glm/gtx/quaternion.hpp"

#include "UUID.h"
#include "../Renderer/Mesh.h"
#include "../Renderer/Material.h"

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

		void CalculateMatrix() {
			m_Mat = glm::translate(glm::mat4(1.0f), m_Position)
				* glm::toMat4(glm::quat(glm::radians(m_Rotation)))
				* glm::scale(glm::mat4(1.0f), m_Scale);
		}

	private:
		glm::mat4 m_Mat = glm::mat4(1.0f);
		glm::vec3 m_Position = glm::vec3();
		glm::vec3 m_Rotation = glm::vec3();
		glm::vec3 m_Scale = glm::vec3(1.0f);
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

		inline std::string GetUUID() { return m_UUID.m_UUIDAsString; }

	private:
		UUID m_UUID;
	};

	struct TagComponent {
		TagComponent() = default;
		TagComponent(std::string& tag)
			: m_Tag(tag) {
		}
		TagComponent(const char* tag)
			: m_Tag(tag) {
		}
		TagComponent(const TagComponent& other) = default;

		inline std::string GetTag() { return m_Tag; }
		inline void SetTag(std::string& tag) { m_Tag = tag; }
		inline void SetTag(char* tag) { m_Tag = tag; }

	private:
		std::string m_Tag = "Empty Entity";
	};
}