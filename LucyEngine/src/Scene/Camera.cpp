#include "Camera.h"
#include "../Renderer/Renderer.h"

namespace Lucy {

	Camera::Camera(glm::vec3& position, float farPlane, float nearPlane, float fov)
		: m_Position(position), m_FarPlane(farPlane), m_NearPlane(nearPlane), m_Fov(fov) {
	}

	EditorCamera::EditorCamera(float farPlane, float nearPlane, float fov)
		: Camera(glm::vec3(0.0f), farPlane, nearPlane, fov) {
	}

	EditorCamera::EditorCamera(glm::vec3& position, float farPlane, float nearPlane, float fov)
		: Camera(position, farPlane, nearPlane, fov) {
	}

	void EditorCamera::UpdateView() {
		//float radius = 10.0f;
		//float x = sin(glfwGetTime()) * radius;
		//float z = cos(glfwGetTime()) * radius;
		m_ViewMatrix = glm::mat4(1.0f);
		m_ViewMatrix = glm::lookAt(m_Position, m_Position + m_ForwardDir, m_UpDir);
	}

	void EditorCamera::UpdateProjection() {
		auto [width, height] = Renderer::GetViewportSize();
		m_Projection = glm::mat4(1.0f);
		m_Projection = glm::perspective(glm::radians(m_Fov), (float)width / height, m_NearPlane, m_FarPlane);
	}
}