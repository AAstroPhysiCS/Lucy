#pragma once

#include <iostream>

#include "glm/glm.hpp"

#include "UUID.h"

namespace Lucy {

	struct TransformComponent {

		TransformComponent() = default;
		TransformComponent(glm::mat4& mat)
			: m_Mat(mat) {
		}
		TransformComponent(const TransformComponent& other) = default;

	private:
		glm::mat4 m_Mat = glm::mat4();
		glm::vec3 m_Position = glm::vec3();
		glm::vec3 m_Rotation = glm::vec3();
		glm::vec3 m_Scale = glm::vec3();
	};

	struct MeshComponent {

		MeshComponent(std::string& path)
			: m_Path(path) {
		}
		MeshComponent(const MeshComponent& other) = default;

	private:
		std::string m_Path;
	};

	struct UUIDComponent {

		UUIDComponent() = default;
		UUIDComponent(UUID& uuid)
			: m_UUID(uuid) {
		}
		UUIDComponent(const UUIDComponent& other) = default;

		std::string& GetUUID() {
			return m_UUID.m_UUIDAsString;
		}

	private:
		UUID m_UUID;

	};

	struct TagComponent {

		TagComponent() = default;
		TagComponent(std::string& tag)
			: m_Tag(tag) {
		}
		TagComponent(const TagComponent& other) = default;

		std::string& GetTag() {
			return m_Tag;
		}

	private:
		std::string m_Tag = "Empty Entity";
	};

}

