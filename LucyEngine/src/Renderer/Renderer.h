#pragma once

#include "../Core/Base.h"
#include "../Core/Window.h"

#include "Scene/Scene.h"
#include "Shader/Shader.h"
#include "Mesh.h"

#include "Context/RenderContext.h"
#include "Context/RendererAPI.h"

namespace Lucy {

	class Renderer {
		using Func = std::function<void()>;

	public:
		static void Init(RefLucy<Window> window, RenderArchitecture renderArchitecture);
		static void Destroy();

		static void SetViewportMousePosition(float x, float y);

		static void BeginScene(Scene& scene);
		static void EndScene();
		static void Submit(const Func&& func);
		static void SubmitMesh(RefLucy<Mesh> mesh, const glm::mat4& entityTransform);

		static void OnFramebufferResize(float sizeX, float sizeY);
		static Entity OnMousePicking();

		static void Dispatch();
		static void ClearDrawCommands();

		inline static auto GetWindowSize() {
			struct Size { int32_t Width, Height; };
			return Size{ s_Window->GetWidth(), s_Window->GetHeight() };
		}

		inline static auto GetViewportSize() { return s_RendererAPI->GetViewportSize(); }
		inline static RenderArchitecture GetCurrentRenderArchitecture() { return s_SelectedArchitecture; }
		inline static ShaderLibrary& GetShaderLibrary() { return s_RendererAPI->GetShaderLibrary(); }
		inline static Scene* GetActiveScene() { return s_RendererAPI->m_ActiveScene; }

		inline static RefLucy<RendererAPI> GetCurrentRenderer() { return s_RendererAPI; }
	private:
		Renderer() = delete;
		~Renderer() = delete;

		static RefLucy<Window> s_Window;
		static RefLucy<RendererAPI> s_RendererAPI;

		static RenderArchitecture s_SelectedArchitecture;

		friend class RenderCommand;
		friend class Input;
		friend class Material;
		
		friend class OpenGLRenderer;

		friend class VulkanDevice;
		friend class VulkanContext;
		friend class VulkanSwapChain;
	};
}