#include "lypch.h"

#include "Camera.h"

#include "Events/Event.h"
#include "Events/InputEvent.h"
#include "Core/Input.h"

namespace Lucy {

	Camera::Camera(int32_t viewportWidth, int32_t viewportHeight, float farPlane, float nearPlane, float fov)
		: m_ViewportWidth(viewportWidth), m_ViewportHeight(viewportHeight), m_FarPlane(farPlane), m_NearPlane(nearPlane), m_Fov(fov) {
	}

	Camera::Camera(const glm::vec3& position, float farPlane, float nearPlane, float fov)
		: m_Position(position), m_FarPlane(farPlane), m_NearPlane(nearPlane), m_Fov(fov) {
	}

	EditorCamera::EditorCamera(int32_t viewportWidth, int32_t viewportHeight, float farPlane, float nearPlane, float fov)
		: Camera(viewportWidth, viewportHeight, farPlane, nearPlane, fov) {
	}

	EditorCamera::EditorCamera(float farPlane, float nearPlane, float fov)
		: Camera(glm::vec3(0.0f), farPlane, nearPlane, fov) {
	}

	EditorCamera::EditorCamera(glm::vec3& position, float farPlane, float nearPlane, float fov)
		: Camera(position, farPlane, nearPlane, fov) {
	}

	void EditorCamera::Update() {
		UpdateValues();

		UpdateView();
		UpdateProjection();
	}

	void EditorCamera::UpdateValues() {
		auto [x, y] = Input::GetMousePosition();

		static double lastX = x;
		static double lastY = y;

		float xOffset = (float) ((x - lastX) * m_Sensivity);
		float yOffset = (float) ((y - lastY) * m_Sensivity);

		lastX = x;
		lastY = y;

		if (Input::IsMousePressed(MouseCode::Button1)) {
			m_Rotation.x += xOffset;
			m_Rotation.y += yOffset;
			m_Rotation.y = glm::clamp(m_Rotation.y, -90.0f, 90.0f);
		}

		float rad90 = glm::radians(m_Rotation.x + 90);
		float rad = glm::radians(m_Rotation.x);

		if (Input::IsKeyPressed(KeyCode::W)) {
			m_Position.x -= glm::cos(rad90) * m_CameraSpeed;
			m_Position.z -= glm::sin(rad90) * m_CameraSpeed;
		}
		if (Input::IsKeyPressed(KeyCode::S)) {
			m_Position.x += glm::cos(rad90) * m_CameraSpeed;
			m_Position.z += glm::sin(rad90) * m_CameraSpeed;
		}
		if (Input::IsKeyPressed(KeyCode::D)) {
			m_Position.x += glm::cos(rad) * m_CameraSpeed;
			m_Position.z += glm::sin(rad) * m_CameraSpeed;
		}
		if (Input::IsKeyPressed(KeyCode::A)) {
			m_Position.x -= glm::cos(rad) * m_CameraSpeed;
			m_Position.z -= glm::sin(rad) * m_CameraSpeed;
		}

		if (Input::IsKeyPressed(KeyCode::LeftShift)) {
			m_Position.y -= m_CameraSpeed;
		} else if (Input::IsKeyPressed(KeyCode::Space)) {
			m_Position.y += m_CameraSpeed;
		}
	}

	void EditorCamera::UpdateView() {
		m_ViewMatrix = glm::mat4(1.0f);
		m_ViewMatrix = glm::rotate(m_ViewMatrix, glm::radians(m_Rotation.y), glm::vec3(1.0f, 0.0f, 0.0f));
		m_ViewMatrix = glm::rotate(m_ViewMatrix, glm::radians(m_Rotation.x), glm::vec3(0.0f, 1.0f, 0.0f));
		m_ViewMatrix = glm::translate(m_ViewMatrix, -m_Position);
	}

	void EditorCamera::UpdateProjection() {
		if (m_ViewportWidth == 0 && m_ViewportHeight == 0) 
			LUCY_ASSERT(false);
		m_Projection = glm::mat4(1.0f);
		m_Projection = glm::perspective(glm::radians(m_Fov), (float)m_ViewportWidth / m_ViewportHeight, m_NearPlane, m_FarPlane);
	}
}