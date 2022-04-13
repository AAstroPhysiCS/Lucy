#pragma once

#include "../../Core/Base.h"
#include "../DrawCommand.h"
#include "RenderContext.h"
#include "../RenderQueue.h"

#include "Scene/Entity.h"
#include "../Shader/Shader.h"

#include "glad/glad.h"

namespace Lucy {

	//Vulkan error codes
	//OpenGL adapts these error codes
	enum class PresentResult {
		SUCCESS = 0,
		NOT_READY = 1,
		TIMEOUT = 2,
		EVENT_SET = 3,
		EVENT_RESET = 4,
		INCOMPLETE = 5,
		ERROR_OUT_OF_HOST_MEMORY = -1,
		ERROR_OUT_OF_DEVICE_MEMORY = -2,
		ERROR_INITIALIZATION_FAILED = -3,
		ERROR_DEVICE_LOST = -4,
		ERROR_MEMORY_MAP_FAILED = -5,
		ERROR_LAYER_NOT_PRESENT = -6,
		ERROR_EXTENSION_NOT_PRESENT = -7,
		ERROR_FEATURE_NOT_PRESENT = -8,
		ERROR_INCOMPATIBLE_DRIVER = -9,
		ERROR_TOO_MANY_OBJECTS = -10,
		ERROR_FORMAT_NOT_SUPPORTED = -11,
		ERROR_SURFACE_LOST_KHR = -1000000000,
		SUBOPTIMAL_KHR = 1000001003,
		ERROR_OUT_OF_DATE_KHR = -1000001004,
		ERROR_INCOMPATIBLE_DISPLAY_KHR = -1000003001,
		ERROR_NATIVE_WINDOW_IN_USE_KHR = -1000000001,
		ERROR_VALIDATION_FAILED_EXT = -1000011001,
	};

	enum class RenderArchitecture;

	class RHI {
	public:
		static RefLucy<RHI> Create(RenderArchitecture architecture);
		virtual void Init() = 0;

		virtual void ClearCommands() = 0;
		virtual void Destroy() = 0;
		virtual void Dispatch() = 0;
		virtual void Submit(const Func&& func) = 0;
		virtual void SubmitMesh(RefLucy<Pipeline> pipeline, RefLucy<Mesh> mesh, const glm::mat4& entityTransform) = 0;
		virtual void SubmitRenderCommand(const RenderCommand& renderCommand) = 0;

		virtual void BeginScene(Scene& scene) = 0;
		virtual PresentResult RenderScene() = 0;
		virtual void EndScene() = 0;

		virtual void OnViewportResize() = 0;
		void SetViewportMousePosition(float x, float y);

		virtual Entity OnMousePicking() = 0;

		inline RenderArchitecture GetCurrentRenderArchitecture() { return m_Architecture; }

		inline void SetViewportSize(int32_t width, int32_t height) const {
			m_ViewportWidth = width;
			m_ViewportHeight = height;
		}

		inline void SetViewportMouse(float viewportMouseX, float viewportMouseY) const {
			m_ViewportMouseX = viewportMouseX;
			m_ViewportMouseY = viewportMouseY;
		}

		inline auto GetViewportSize() {
			struct Size { int32_t Width, Height; };
			return Size{ m_ViewportWidth, m_ViewportHeight };
		}

		inline ShaderLibrary& GetShaderLibrary() { return m_ShaderLibrary; }
	protected:
		RHI(RenderArchitecture renderArchitecture);
		~RHI() = default;

		mutable int32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
		mutable float m_ViewportMouseX = 0, m_ViewportMouseY = 0;

		Scene* m_ActiveScene = nullptr;

		std::vector<Func> m_RenderFunctions;
		std::vector<MeshDrawCommand> m_MeshDrawCommands;
		RenderCommandQueue m_RenderCommandQueue;

		RefLucy<RenderContext> m_RenderContext;
		RenderArchitecture m_Architecture;

		ShaderLibrary m_ShaderLibrary;

		friend class Renderer;
		friend class Material;
		friend class ImGuiLayer; //for m_RenderContext
		friend class VulkanAllocator; //for m_RenderContext
	};
}