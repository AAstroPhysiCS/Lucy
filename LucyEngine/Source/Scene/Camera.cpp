#include "lypch.h"

#include "Camera.h"

#include "Events/EventHandler.h"

namespace Lucy {

	Camera::Camera(const glm::vec3& position, const glm::vec3& rotation, float nearPlane, float farPlane)
		: m_Position(position), m_Rotation(rotation), m_NearPlane(nearPlane), m_FarPlane(farPlane) {
	}

	Camera::Camera(const glm::vec3& position, float nearPlane, float farPlane)
		: m_Position(position), m_NearPlane(nearPlane), m_FarPlane(farPlane) {
	}

	Camera::Camera(float nearPlane, float farPlane) 
		: m_NearPlane(nearPlane), m_FarPlane(farPlane) {
	}

	void Camera::SetPosition(const glm::vec3& position) {
		m_Position = position;
		UpdateView();
	}

	void Camera::SetRotation(const glm::vec3& rotation) {
		m_Rotation = rotation;
		UpdateView();
	}

	CameraViewProjection Camera::GetCameraViewProjection() const {
		CameraViewProjection mvp;
		mvp.View = m_ViewMatrix;
		mvp.Proj = m_Projection;
		mvp.CamPos = glm::vec4(m_Position, 1.0f);

		return mvp;
	}

	PerspectiveCamera::PerspectiveCamera(const glm::vec3& m_Position, const glm::vec3& m_Rotation, float nearPlane, float farPlane, float fov) 
		: Camera(m_Position, m_Rotation, nearPlane, farPlane), m_Fov(fov) {
	}

	PerspectiveCamera::PerspectiveCamera(const glm::vec3& m_Position, float nearPlane, float farPlane, float fov)
		: Camera(m_Position, nearPlane, farPlane), m_Fov(fov) {
	}

	PerspectiveCamera::PerspectiveCamera(float nearPlane, float farPlane, float fov)
		: Camera(nearPlane, farPlane), m_Fov(fov) {
	}

	void PerspectiveCamera::SetAspectRatio(float aspectRatio) {
		m_AspectRatio = aspectRatio;
		UpdateProjection();
	}

	void PerspectiveCamera::UpdateProjection() {
		if (m_AspectRatio == 0)
			return;
		m_Projection = glm::mat4(1.0f);
		m_Projection = glm::perspective(glm::radians(m_Fov), m_AspectRatio, m_NearPlane, m_FarPlane);
	}

	void PerspectiveCamera::Update() {
		UpdateView();
		UpdateProjection();
	}

	OrthographicCamera::OrthographicCamera(const glm::vec3& m_Position, const glm::vec3& m_Rotation, float left, float right, float bottom, float top, float nearPlane, float farPlane)
		: Camera(m_Position, m_Rotation, nearPlane, farPlane), m_Left(left), m_Right(right), m_Bottom(bottom), m_Top(top) {
	}

	OrthographicCamera::OrthographicCamera(const glm::vec3& m_Position, float left, float right, float bottom, float top, float nearPlane, float farPlane)
		: Camera(m_Position, nearPlane, farPlane), m_Left(left), m_Right(right), m_Bottom(bottom), m_Top(top) {
	}

	OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top, float nearPlane, float farPlane) 
		: Camera(nearPlane, farPlane), m_Left(left), m_Right(right), m_Bottom(bottom), m_Top(top) {
	}

	void OrthographicCamera::Update() {
		UpdateView();
		UpdateProjection();
	}

	void OrthographicCamera::UpdateProjection() {
		m_Projection = glm::mat4(1.0f);
		m_Projection = glm::ortho(m_Left, m_Right, m_Bottom, m_Top, m_NearPlane, m_FarPlane);
	}

	EditorCamera::EditorCamera(const glm::vec3& m_Position, const glm::vec3& m_Rotation, float nearPlane, float farPlane, float fov) 
		: PerspectiveCamera(m_Position, m_Rotation, nearPlane, farPlane, fov) {
	}

	EditorCamera::EditorCamera(const glm::vec3& m_Position, float nearPlane, float farPlane, float fov)
		: PerspectiveCamera(m_Position, nearPlane, farPlane, fov) {
	}

	EditorCamera::EditorCamera(float nearPlane, float farPlane, float fov)
		: PerspectiveCamera(nearPlane, farPlane, fov) {
	}

	void EditorCamera::UpdateMovement() {
		auto x = Input::GetMouseX();
		auto y = Input::GetMouseY();

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
		UpdateMovement();

		m_ViewMatrix = glm::mat4(1.0f);
		m_ViewMatrix = glm::rotate(m_ViewMatrix, glm::radians(m_Rotation.y), glm::vec3(1.0f, 0.0f, 0.0f));
		m_ViewMatrix = glm::rotate(m_ViewMatrix, glm::radians(m_Rotation.x), glm::vec3(0.0f, 1.0f, 0.0f));
		m_ViewMatrix = glm::translate(m_ViewMatrix, -m_Position);
	}
}