#pragma once

#include "../Core/Base.h"
#include "../Core/Window.h"

#include "Scene/Scene.h"
#include "Shader/Shader.h"
#include "Mesh.h"

#include "DrawCommand.h"

#include "Context/RHI.h"
#include "Context/RenderContext.h"

namespace Lucy {

	struct ImGuiPipeline;

	struct RendererSpecification {
		RenderArchitecture Architecture;
		RefLucy<Window> Window = nullptr;
	};

	class Renderer {
	public:
		static void Init(const RendererSpecification& specs);
		static void Destroy();

		static void BeginScene(Scene& scene);
		static void RenderScene();
		static PresentResult EndScene();

		static void Enqueue(const SubmitFunc&& func);
		static void EnqueueStaticMesh(RefLucy<Mesh> mesh, const glm::mat4& entityTransform);

		static void RecordToCommandQueue(RecordFunc<>&& func);
		static void RecordToCommandQueue(RecordFunc<MeshDrawCommand>&& func);

		static void SetUIDrawData(std::function<void(VkCommandBuffer commandBuffer)>&& func);
		static void UIPass(const ImGuiPipeline& imguiPipeline);

		static void BindPipeline(RefLucy<Pipeline> pipeline);
		static void UnbindPipeline();
		static void BindBuffers(RefLucy<Mesh> mesh);
		static void BindBuffers(RefLucy<VertexBuffer> vertexBuffer, RefLucy<IndexBuffer> indexBuffer);
		static void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
								int32_t vertexOffset, uint32_t firstInstance);

		static void OnWindowResize();
		static void OnViewportResize();
		static Entity OnMousePicking();

		static void Dispatch();
		static void ClearQueues();

		static void SetViewportSize(int32_t width, int32_t height);
		static void SetViewportMouse(float viewportMouseX, float viewportMouseY);

		inline static RefLucy<Window> GetWindow() { return s_Specs.Window; }
		inline static auto GetWindowSize() {
			struct Size { int32_t Width, Height; };
			return Size{ s_Specs.Window->GetWidth(), s_Specs.Window->GetHeight() };
		}

		inline static auto GetViewportSize() { return s_RHI->GetViewportSize(); }
		inline static RenderArchitecture GetCurrentRenderArchitecture() { return s_Specs.Architecture; }
		inline static ShaderLibrary& GetShaderLibrary() { return s_ShaderLibrary; }
		inline static RefLucy<RHI> GetCurrentRenderer() { return s_RHI; }
	private:
		Renderer() = delete;
		~Renderer() = delete;
		
		static RefLucy<RHI> s_RHI;
		static RefLucy<Pipeline> s_ActivePipeline;
		static RendererSpecification s_Specs;

		static ShaderLibrary s_ShaderLibrary;

		static std::function<void(VkCommandBuffer commandBuffer)> s_UIDrawDataFunc;

		friend class ViewportRenderer; //for s_UIDrawDataFunc
	};
}