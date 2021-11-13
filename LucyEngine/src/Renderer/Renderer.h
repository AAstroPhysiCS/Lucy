#pragma once

#include "../Core/Base.h"
#include "../Scene/Scene.h"
#include "../Scene/Camera.h"
#include "../Core/Window.h"

#include "Context/RendererAPI.h"
#include "Context/RenderContext.h"

#include "DrawCommand.h"

namespace Lucy {

	class RenderPass;
	class FrameBuffer;
	class ShaderLibrary;

	class Renderer {
		using Func = std::function<void()>;

	public:
		static void Init(RefLucy<Window> window, RenderAPI rendererContext);
		static void Destroy();

		inline static RenderAPI GetCurrentRenderAPI() { return s_RenderContext->GetRenderAPI(); }
		inline static RefLucy<RenderPass>& GetGeometryPass() { return s_GeometryPass; }
		inline static ShaderLibrary& GetShaderLibrary() { return s_ShaderLibrary; }

		static void SetViewportSize(int32_t width, int32_t height);

		inline static auto GetViewportSize() {
			struct Size { int32_t Width, Height; };
			return Size{ m_ViewportWidth, m_ViewportHeight };
		}

		inline static auto GetWindowSize() {
			struct Size { int32_t Width, Height; };
			return Size{ s_Window->GetWidth(), s_Window->GetHeight() };
		}

		static void BeginScene(Scene& scene);
		static void EndScene();
		static void Submit(const Func&& func);
		static void SubmitMesh(RefLucy<Mesh>& mesh, const glm::mat4& entityTransform);

		static void GeometryPass();

		static void Dispatch();
		static void ClearDrawCommands();
	private:
		static RefLucy<RendererAPI> s_RendererAPI;
		static RefLucy<RenderContext> s_RenderContext;
		static RefLucy<RenderPass> s_GeometryPass;
		static RefLucy<Window> s_Window;

		static Camera* s_ActiveCamera;

		static std::vector<Func> s_RenderQueue;
		static std::vector<MeshDrawCommand> s_MeshDrawCommand;

		static ShaderLibrary s_ShaderLibrary;

		static int32_t m_ViewportWidth, m_ViewportHeight;

		friend class RenderCommand;

		Renderer() = delete;
		~Renderer() = delete;
	};

}