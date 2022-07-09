#pragma once

#include "../Core/Base.h"
#include "../Core/Window.h"

#include "Shader/Shader.h"

#include "Context/RHI.h"

#include "VulkanDescriptors.h"

namespace Lucy {
	
	class Scene;
	class PushConstant;

	struct ImGuiPipeline;

	enum class GlobalDescriptorSets : uint8_t;

	struct RendererCreateInfo {
		RenderArchitecture Architecture;
		Ref<Window> Window = nullptr;
	};

	class Renderer {
	public:
		static void Init(const RendererCreateInfo& createInfo);
		static void WaitForDevice();
		static void Destroy();

		static void BeginScene(Scene& scene);
		static void RenderScene();
		static PresentResult EndScene();

		static void Enqueue(const SubmitFunc&& func);
		static void EnqueueStaticMesh(Priority priority, Ref<Mesh> mesh, const glm::mat4& entityTransform);

		static void RecordStaticMeshToCommandQueue(Ref<Pipeline> pipeline, RecordFunc<Ref<DrawCommand>>&& func);

		static void SetUIDrawData(std::function<void(VkCommandBuffer commandBuffer)>&& func);
		static void UIPass(const ImGuiPipeline& imguiPipeline);

		static void OnWindowResize();
		static void OnViewportResize();
		static Entity OnMousePicking();

		static void Dispatch();
		static void ClearQueues();

		static void SetViewportSize(int32_t width, int32_t height);
		static void SetViewportMouse(float viewportMouseX, float viewportMouseY);

		inline static Ref<Window> GetWindow() { return s_CreateInfo.Window; }
		inline static auto GetWindowSize() {
			struct Size { int32_t Width, Height; };
			return Size{ s_CreateInfo.Window->GetWidth(), s_CreateInfo.Window->GetHeight() };
		}

		inline static auto GetViewportSize() { return s_RHI->GetViewportSize(); }
		inline static RenderArchitecture GetCurrentRenderArchitecture() { return s_CreateInfo.Architecture; }
		inline static ShaderLibrary& GetShaderLibrary() { return s_ShaderLibrary; }
		inline static Ref<RHI> GetCurrentRenderer() { return s_RHI; }
	public:
		static void BindBuffers(Ref<Mesh> mesh);
		static void BindBuffers(Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer);

		static void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
								int32_t vertexOffset, uint32_t firstInstance);

		static void BindPushConstant(Ref<Pipeline> pipeline, const PushConstant& pushConstant);
	private:
		//for CommandQueue
		static void BindPipeline(Ref<Pipeline> pipeline);

		static void BeginRenderPass(Ref<Pipeline> pipeline, VkCommandBuffer commandBuffer);
		static void EndRenderPass(Ref<Pipeline> pipeline);
		
		static void UpdateResources(const std::vector<Ref<DrawCommand>>& drawCommands, Ref<Pipeline> pipeline);

		static void BindDescriptorSet(Ref<Pipeline> pipeline, Ref<VulkanDescriptorSet> descriptorSet);
		static Ref<VulkanDescriptorSet> GetDescriptorSet(Ref<Pipeline> pipeline, GlobalDescriptorSets descriptorSet);
	private:
		Renderer() = delete;
		~Renderer() = delete;

		static Ref<RHI> s_RHI;
		static RendererCreateInfo s_CreateInfo;

		static ShaderLibrary s_ShaderLibrary;

		static std::function<void(VkCommandBuffer commandBuffer)> s_UIDrawDataFunc;

		friend class ViewportRenderer; //for s_UIDrawDataFunc
		friend class CommandQueue; //for BeginRenderPass and EndRenderPass
	};
}