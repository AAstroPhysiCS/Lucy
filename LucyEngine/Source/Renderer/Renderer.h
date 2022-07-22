#pragma once

#include "Core/Base.h"
#include "Core/Window.h"

#include "Shader/Shader.h"

#include "RenderDevice.h"
#include "Renderer/Descriptors/DescriptorSet.h"
#include "RenderArchitecture.h"

namespace Lucy {
	
	class Scene;
	struct ImGuiPipeline;

	class Renderer {
	public:
		static void Init(RenderArchitecture arch, Ref<Window> window);
		static void WaitForDevice();
		static void Destroy();

		static void BeginScene(Scene& scene);
		static void RenderScene();
		static PresentResult EndScene();

		static void Enqueue(EnqueueFunc&& func);
		static void EnqueueStaticMesh(Priority priority, Ref<Mesh> mesh, const glm::mat4& entityTransform);

		static void RecordStaticMeshToCommandQueue(Ref<Pipeline> pipeline, RecordFunc<VkCommandBuffer, Ref<DrawCommand>>&& func);

		static void OnWindowResize();
		static void OnViewportResize();
		static Entity OnMousePicking();

		static void Dispatch();
		static void ClearQueues();

		static void SetViewportSize(int32_t width, int32_t height);
		static void SetViewportMouse(float viewportMouseX, float viewportMouseY);

		inline static Ref<Window> GetWindow() { return s_Window; }
		inline static auto GetWindowSize() {
			struct Size { int32_t Width, Height; };
			return Size{ s_Window->GetWidth(), s_Window->GetHeight() };
		}

		inline static auto GetViewportSize() { return s_RenderDevice->GetViewportSize(); }
		inline static RenderArchitecture GetCurrentRenderArchitecture() { return s_Arch; }
		inline static ShaderLibrary& GetShaderLibrary() { return s_ShaderLibrary; }
		inline static Ref<RenderDevice> GetCurrentRenderDevice() { return s_RenderDevice; }
	public:
		static void BindBuffers(VkCommandBuffer commandBuffer, Ref<Mesh> mesh);
		static void BindPushConstant(VkCommandBuffer commandBuffer, Ref<Pipeline> pipeline, const PushConstant& pushConstant);
		static void BindDescriptorSet(void* commandBufferHandle, Ref<Pipeline> pipeline, Ref<DescriptorSet> descriptorSet);
		//for CommandQueue
		static void BindPipeline(VkCommandBuffer commandBuffer, Ref<Pipeline> pipeline);
		static void BeginRenderPass(VkCommandBuffer commandBuffer, Ref<Pipeline> pipeline);
		static void EndRenderPass(Ref<Pipeline> pipeline);
		static void DrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount, 
								uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);
		static void UpdateResources(const std::vector<Ref<DrawCommand>>& drawCommands, Ref<Pipeline> pipeline);
	private:
		Renderer() = delete;
		~Renderer() = delete;

		static void SetUIDrawData(std::function<void(VkCommandBuffer commandBuffer)>&& func);
		static void UIPass(const ImGuiPipeline& imguiPipeline);

		static Ref<RenderDevice> s_RenderDevice;

		static Ref<Window> s_Window;
		static RenderArchitecture s_Arch;

		static ShaderLibrary s_ShaderLibrary;

		static std::function<void(VkCommandBuffer commandBuffer)> s_UIDrawDataFunc;

		friend class ImGuiOverlay; //for SetUIDrawData
		friend class ViewportRenderer; //for s_UIDrawDataFunc
	};
}