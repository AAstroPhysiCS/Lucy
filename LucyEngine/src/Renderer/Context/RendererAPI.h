#pragma once

#include "../../Core/Base.h"
#include "../DrawCommand.h"
#include "RenderContext.h"
#include "Renderer/RenderCommand.h"

#include "Renderer/Buffer/UniformBuffer.h"

#include "Scene/Entity.h"
#include "../Shader/Shader.h"

#include "glad/glad.h"

namespace Lucy {

	enum class RenderArchitecture;

	class RendererAPI {
	public:
		static RefLucy<RendererAPI> Create(RenderArchitecture architecture);
		virtual void Init();

		virtual void Draw() = 0;
		virtual void ClearCommands() = 0;
		virtual void Destroy() = 0;
		virtual void Dispatch() = 0;
		virtual void Submit(const Func&& func) = 0;
		virtual void SubmitMesh(RefLucy<Mesh> mesh, const glm::mat4& entityTransform) = 0;

		virtual void BeginScene(Scene& scene) = 0;
		virtual void EndScene() = 0;

		void SetViewportMousePosition(float x, float y);

		virtual void OnFramebufferResize(float sizeX, float sizeY) = 0;
		virtual Entity OnMousePicking() = 0;

		inline RenderArchitecture GetCurrentRenderArchitecture() { return m_Architecture; }

		inline auto GetViewportSize() {
			struct Size { int32_t Width, Height; };
			return Size{ m_ViewportWidth, m_ViewportHeight };
		}

		inline ShaderLibrary& GetShaderLibrary() { return m_ShaderLibrary; }
	protected:
		RendererAPI(RenderArchitecture renderArchitecture);
		~RendererAPI() = default;

		int32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
		float m_ViewportMouseX = 0, m_ViewportMouseY = 0;

		Scene* m_ActiveScene = nullptr;

		std::vector<Func> m_RenderQueue;
		std::vector<MeshDrawCommand> m_MeshDrawCommand;

		RefLucy<RenderContext> m_RenderContext;
		RefLucy<RenderCommand> m_RenderCommand;

		RefLucy<UniformBuffer> m_CameraUniformBuffer;
		RefLucy<UniformBuffer> m_TextureSlotsUniformBuffer;
		RenderArchitecture m_Architecture;

		ShaderLibrary m_ShaderLibrary;

		friend class Renderer;
		friend class Material;
	};
}