#pragma once

#include "Utilities/UUID.h"

#include "Renderer/Mesh.h"
#include "Renderer/Image/Image.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	class Entity;

	struct TransformComponent {
		TransformComponent() = default;
		TransformComponent(const glm::mat4& mat)
			: m_Mat(mat) {
		}
		TransformComponent(const TransformComponent& other) = default;

		const glm::mat4& GetMatrix() const { return m_Mat; }
		const glm::vec3& GetPosition() const { return m_Position; }
		const glm::vec3& GetRotation() const { return m_Rotation; }
		const glm::vec3& GetScale() const { return m_Scale; }
		
		glm::mat4& GetMatrix() { return m_Mat; }
		glm::vec3& GetPosition() { return m_Position; }
		glm::vec3& GetRotation() { return m_Rotation; }
		glm::vec3& GetScale() { return m_Scale; }

		void CalculateMatrix();

		inline bool IsValid() const { return true; }
	private:
		glm::mat4 m_Mat = glm::mat4(1.0f);
		glm::vec3 m_Position = glm::vec3();
		glm::vec3 m_Rotation = glm::vec3();
		glm::vec3 m_Scale = glm::vec3(1.0f);
	};

	struct MeshComponent {
		MeshComponent() = default;
		MeshComponent(const std::string& path)
			: m_Mesh(Memory::CreateRef<Mesh>(path)) {
		}
		MeshComponent(const MeshComponent& other) = default;

		void LoadMesh(const Entity& e, const std::string& path);

		inline Ref<Mesh> GetMesh() { return m_Mesh; }

		void SetObjectHandle(const RenderDeviceObjectHandle& handle) { m_Handle = handle; }
		const RenderDeviceObjectHandle& GetRenderDeviceObjectHandle() const { return m_Handle; }

		inline bool IsValid() { return m_Mesh.get() != nullptr; }
	private:
		Ref<Mesh> m_Mesh = nullptr;

		RenderDeviceObjectHandle m_Handle;
	};

	struct UUIDComponent {
		UUIDComponent() = default;
		UUIDComponent(const UUID& uuid)
			: m_UUID(uuid) {
		}
		UUIDComponent(const UUIDComponent& other) = default;

		inline std::string GetUUID() const { return m_UUID.GetID(); }
		inline bool IsValid() const { return m_UUID != "0"; }
	private:
		UUID m_UUID;
	};

	struct TagComponent {
		TagComponent() = default;
		TagComponent(const std::string& tag)
			: m_Tag(tag) {
		}
		TagComponent(const char* tag)
			: m_Tag(tag) {
		}
		TagComponent(const TagComponent& other) = default;

		inline const std::string& GetTag() const { return m_Tag; }
		inline void SetTag(const std::string& tag) { m_Tag = tag; }
		inline void SetTag(const char* tag) { m_Tag = tag; }

		inline bool IsValid() const { return m_Tag != "Empty Entity"; }
	private:
		std::string m_Tag = "Empty Entity";
	};

	struct DirectionalLightComponent {
		DirectionalLightComponent() = default;
		DirectionalLightComponent(const glm::vec3& direction, const glm::vec3 color)
			: m_Direction(direction), m_Color(color) {
		}
		DirectionalLightComponent(const DirectionalLightComponent& other) = default;

		inline glm::vec3& GetDirection() { return m_Direction; }
		inline glm::vec3& GetColor() { return m_Color; }

		inline bool IsValid() const { return true; }
	private:
		glm::vec3 m_Direction = glm::vec3(1.0f);
		glm::vec3 m_Color = glm::vec3(1.0f);
	};

	struct HDRCubemapComponent {
		HDRCubemapComponent() = default;
		HDRCubemapComponent(const HDRCubemapComponent& other) = default;

		void LoadCubemap(const std::filesystem::path& path);
		
		bool IsPrimary = false;

		inline bool IsValid() const { return true; }
		inline const std::filesystem::path& GetPath() const { return m_Path; }

		inline Ref<Image> GetIrradianceImage() const { return Renderer::AccessResource<Image>(m_IrradianceImageHandle); }
	private:
		std::filesystem::path m_Path;

		RenderDeviceResourceHandle m_OriginalImageHandle{};
		RenderDeviceResourceHandle m_IrradianceImageHandle{};
	};
}