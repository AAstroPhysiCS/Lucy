#pragma once

namespace Lucy {

	struct CameraViewProjection {
		glm::mat4 View;
		glm::mat4 Proj;
		glm::vec4 CamPos;
	};

	class Camera {
	public:
		static inline constexpr const glm::vec3 s_RightDir = glm::vec3(1.0f, 0.0f, 0.0f);
		static inline constexpr const glm::vec3 s_UpDir = glm::vec3(0.0f, 1.0f, 0.0f);
		static inline constexpr const glm::vec3 s_ForwardDir = glm::vec3(0.0f, 0.0f, 1.0f);

		Camera(const glm::vec3& position, const glm::vec3& rotation, float nearPlane, float farPlane);
		Camera(const glm::vec3& position, float nearPlane, float farPlane);
		Camera(float nearPlane, float farPlane);
		Camera() = default;
		virtual ~Camera() = default;

		void SetPosition(const glm::vec3& position);
		void SetRotation(const glm::vec3& rotation);

		inline const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
		inline const glm::mat4& GetProjectionMatrix() const { return m_Projection; }

		inline const glm::vec3& GetPosition() const { return m_Position; }
		inline const glm::vec3& GetRotation() const { return m_Rotation; }

		inline float GetNearPlane() const { return m_NearPlane; }
		inline float GetFarPlane() const { return m_FarPlane; }

		CameraViewProjection GetCameraViewProjection() const;

		virtual void Update() = 0;
	protected:
		virtual void UpdateView() = 0;

		glm::mat4 m_ViewMatrix = glm::mat4(1.0f);
		glm::mat4 m_Projection = glm::mat4(1.0f);

		glm::vec3 m_Position = glm::vec3(0.0f);
		glm::vec3 m_Rotation = glm::vec3(0.0f);

		float m_NearPlane = 0.0f, m_FarPlane = 0.0f;
	};

	class PerspectiveCamera : public Camera {
	public:
		PerspectiveCamera(const glm::vec3& position, const glm::vec3& rotation, float nearPlane, float farPlane, float fov);
		PerspectiveCamera(const glm::vec3& position, float nearPlane, float farPlane, float fov);
		PerspectiveCamera(float nearPlane, float farPlane, float fov);
		virtual ~PerspectiveCamera() = default;

		inline float GetFov() const { return m_Fov; }

		void SetAspectRatio(float aspectRatio);

		void Update() final override;
	private:
		void UpdateProjection();

		float m_AspectRatio = 0.0f;
		float m_Fov = 0.0f;
	};

	class OrthographicCamera : public Camera {
	public:
		OrthographicCamera(const glm::vec3& position, const glm::vec3& rotation, float left, float right, float bottom, float top, float nearPlane, float farPlane);
		OrthographicCamera(const glm::vec3& position, float left, float right, float bottom, float top, float nearPlane, float farPlane);
		OrthographicCamera(float left, float right, float bottom, float top, float nearPlane, float farPlane);
		virtual ~OrthographicCamera() = default;

		void Update() final override;
	protected:
		float m_Left = 0.0f, m_Right = 0.0f, m_Bottom = 0.0f, m_Top = 0.0f;
	private:
		void UpdateProjection();
	};

	class EditorCamera : public PerspectiveCamera {
	public:
		EditorCamera(const glm::vec3& position, const glm::vec3& rotation, float nearPlane, float farPlane, float fov);
		EditorCamera(const glm::vec3& position, float nearPlane, float farPlane, float fov);
		EditorCamera(float nearPlane, float farPlane, float fov);
		virtual ~EditorCamera() = default;
	private:
		float m_CameraSpeed = 0.025f;
		float m_Sensivity = 0.3f;

		void UpdateMovement();
		void UpdateView() final override;
	};
}

