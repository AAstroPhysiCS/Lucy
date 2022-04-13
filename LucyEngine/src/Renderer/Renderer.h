#pragma once

#include "../Core/Base.h"
#include "../Core/Window.h"

#include "Scene/Scene.h"
#include "Shader/Shader.h"
#include "Mesh.h"

#include "Context/RenderContext.h"
#include "Context/RHI.h"

namespace Lucy {

	class RenderCommand;

	class Renderer {
	public:
		static void Init(RefLucy<Window> window, RenderArchitecture renderArchitecture);
		static void Destroy();

		static void SetViewportMousePosition(float x, float y);

		static void BeginScene(Scene& scene);
		static void EndScene();
		static PresentResult RenderScene();

		static void Submit(const Func&& func);
		static void SubmitUIPass(const std::function<void(VkCommandBuffer commandBuffer)>&& func);
		static void SubmitMesh(RefLucy<Pipeline> pipeline, RefLucy<Mesh> mesh, const glm::mat4& entityTransform);
		static void SubmitRenderCommand(const RenderCommand& renderCommand);

		static void OnViewportResize();
		static Entity OnMousePicking();

		static void Dispatch();
		static void ClearDrawCommands();

		inline static void SetViewportSize(int32_t width, int32_t height);
		inline static void SetViewportMouse(float viewportMouseX, float viewportMouseY);

		inline static auto GetWindowSize() {
			struct Size { int32_t Width, Height; };
			return Size{ s_Window->GetWidth(), s_Window->GetHeight() };
		}

		inline static auto GetViewportSize() { return s_RendererAPI->GetViewportSize(); }
		inline static RenderArchitecture GetCurrentRenderArchitecture() { return s_SelectedArchitecture; }
		inline static ShaderLibrary& GetShaderLibrary() { return s_RendererAPI->GetShaderLibrary(); }
		inline static Scene* GetActiveScene() { return s_RendererAPI->m_ActiveScene; }
		inline static RefLucy<RHI> GetCurrentRenderer() { return s_RendererAPI; }
	private:
		Renderer() = delete;
		~Renderer() = delete;

		static RefLucy<Window> s_Window;
		static RefLucy<RHI> s_RendererAPI;

		static RenderArchitecture s_SelectedArchitecture;
		inline static std::function<void(VkCommandBuffer commandBuffer)> s_UIPassFunc;

		friend class RenderCommand;
		friend class Input;
		friend class Material;
		
		friend class OpenGLRHI;

		friend class VulkanDevice;
		friend class VulkanContext;
		friend class VulkanSwapChain;
		friend class ViewportRenderer; //for s_UIPassFunc

		friend class ImGuiLayer; //for SubmitUIPass
	};
}