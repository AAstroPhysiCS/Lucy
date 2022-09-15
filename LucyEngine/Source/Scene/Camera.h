#pragma once

#include "glm/gtx/quaternion.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

namespace Lucy {

	struct VP {
		glm::mat4 View;
		glm::mat4 Proj;
		alignas(4) glm::vec3 CamPos;
	};

	class Camera {
	protected:
		Camera(int32_t m_ViewportWidth, int32_t m_ViewportHeight, float farPlane, float nearPlane, float fov);
		Camera(const glm::vec3& position, float farPlane, float nearPlane, float fov);
		virtual ~Camera() = default;

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
		void SetPosition(const glm::vec3& position) { m_Position = position; }
		void SetRotation(const glm::vec3& rotation) { m_Rotation = rotation; }

		void SetViewportSize(int32_t viewportWidth, int32_t viewportHeight) { 
			m_ViewportWidth = viewportWidth; 
			m_ViewportHeight = viewportHeight;
		}

		inline glm::vec3& GetPosition() { return m_Position; }
		inline glm::vec3& GetRotation() { return m_Rotation; }
		inline glm::mat4& GetViewMatrix() { return m_ViewMatrix; }
		inline glm::mat4& GetProjectionMatrix() { return m_Projection; }

		inline VP GetVP() {
			VP mvp;
			mvp.View = m_ViewMatrix;
			mvp.Proj = m_Projection;
			mvp.Proj[1][1] *= -1;
			mvp.CamPos = m_Position;

			return mvp;
		}

		virtual void UpdateView() = 0;
		virtual void UpdateProjection() = 0;
	};

	class EditorCamera : public Camera {
	public:
		EditorCamera(float farPlane = 10000.0f, float nearPlane = 0.01f, float fov = 90.0f);
		EditorCamera(int32_t m_ViewportWidth, int32_t m_ViewportHeight, float farPlane = 10000.0f, float nearPlane = 0.01f, float fov = 90.0f);
		EditorCamera(glm::vec3& position, float farPlane = 10000.0f, float nearPlane = 0.01f, float fov = 90.0f);
		virtual ~EditorCamera() = default;

		void Update();
	private:
		void UpdateValues();

		float m_CameraSpeed = 0.025f;
		float m_Sensivity = 0.3f;

		void UpdateView() final override;
		void UpdateProjection() final override;
	};
}

