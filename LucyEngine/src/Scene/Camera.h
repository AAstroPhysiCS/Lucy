#pragma once

#include "Events/Event.h"
#include "Events/InputEvent.h"
#include "Core/Metrics.h"

#include "glm/gtx/quaternion.hpp"

namespace Lucy {

	class Camera {
	protected:
		Camera(int32_t m_ViewportWidth, int32_t m_ViewportHeight, float farPlane, float nearPlane, float fov);
		Camera() = default;
		Camera(glm::vec3& position, float farPlane, float nearPlane, float fov);

		glm::mat4 m_ViewMatrix = glm::mat4(1.0f);
		glm::mat4 m_Projection = glm::mat4(1.0f);
		glm::vec3 m_Position = glm::vec3(0.0f);
		glm::vec3 m_Rotation = glm::vec3(0.0f);

		const glm::vec3 m_RightDir = glm::vec3(1.0f, 0.0f, 0.0f);
		const glm::vec3 m_UpDir = glm::vec3(0.0f, 1.0f, 0.0f);
		const glm::vec3 m_ForwardDir = glm::vec3(0.0f, 0.0f, 1.0f);

		float m_FarPlane, m_NearPlane;
		float m_Fov;
		int32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
	public:
		void SetPosition(glm::vec3& position) { m_Position = position; }
		void SetRotation(glm::vec3& rotation) { m_Rotation = rotation; }

		void SetViewportSize(int32_t viewportWidth, int32_t viewportHeight) { 
			m_ViewportWidth = viewportWidth; 
			m_ViewportHeight = viewportHeight;
		}

		inline glm::vec3& GetPosition() { return m_Position; }
		inline glm::vec3& GetRotation() { return m_Rotation; }
		inline glm::mat4& GetViewMatrix() { return m_ViewMatrix; }
		inline glm::mat4& GetProjectionMatrix() { return m_Projection; }

		virtual void OnEvent(PerformanceMetrics& rendererMetrics) = 0;
		virtual void UpdateView() = 0;
		virtual void UpdateProjection() = 0;
	};

	class EditorCamera : public Camera {
	public:
		EditorCamera(int32_t m_ViewportWidth, int32_t m_ViewportHeight, float farPlane = 1000.0f, float nearPlane = 0.1f, float fov = 90.0f);
		EditorCamera(float farPlane = 1000.0f, float nearPlane = 0.1f, float fov = 90.0f);
		EditorCamera(glm::vec3& position, float farPlane = 1000.0f, float nearPlane = 0.1f, float fov = 90.0f);
		virtual ~EditorCamera() = default;

		void OnEvent(PerformanceMetrics& rendererMetrics);
		void Update();
	private:

		float m_CameraSpeed = 0.025f;
		float m_Sensivity = .3f;

		void UpdateView();
		void UpdateProjection();
	};
}

