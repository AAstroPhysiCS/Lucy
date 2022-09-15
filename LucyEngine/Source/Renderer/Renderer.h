#pragma once

#include "Renderer/RendererBase.h"

namespace Lucy {

	class Entity;

	class Renderer final {
	public:
		Renderer() = delete;
		~Renderer() = delete;

		static void Init(RenderArchitecture arch, Ref<Window>& window);

		static void BeginScene(Ref<Scene>& scene);
		static void RenderScene();
		static RenderContextResultCodes EndScene();
		static void WaitForDevice();
		static void Destroy();

		//commandBufferHandle is the handle of the commandBuffer
		//Vulkan: VkCommandBuffer
		static void BindBuffers(void* commandBufferHandle, Ref<Mesh> mesh);
		static void BindPushConstant(void* commandBufferHandle, Ref<GraphicsPipeline> pipeline, const VulkanPushConstant& pushConstant);
		static void BindPipeline(void* commandBufferHandle, Ref<GraphicsPipeline> pipeline);
		static void BindAllDescriptorSets(void* commandBufferHandle, Ref<GraphicsPipeline> pipeline);
		static void UpdateDescriptorSets(Ref<GraphicsPipeline> pipeline);
		static void BindDescriptorSet(void* commandBufferHandle, Ref<GraphicsPipeline> pipeline, uint32_t setIndex);
		static void BindBuffers(void* commandBufferHandle, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer);
		static void DrawIndexed(void* commandBufferHandle, uint32_t indexCount, uint32_t instanceCount,
								 uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);

		static void BeginRenderPass(void* commandBufferHandle, Ref<GraphicsPipeline> pipeline);
		static void EndRenderPass(Ref<GraphicsPipeline> pipeline);

		static void DirectCopyBuffer(VkBuffer& stagingBuffer, VkBuffer& buffer, VkDeviceSize size);
		static void ExecuteSingleTimeCommand(std::function<void(VkCommandBuffer)>&& func);

		static CommandResourceHandle CreateCommandResource(CommandFunc&& func, Ref<GraphicsPipeline> pipeline);

		template <typename T, typename ... Args>
		inline static void EnqueueCommand(CommandResourceHandle resourceHandle, Args&&... args) {
			LUCY_PROFILE_NEW_EVENT("Renderer::EnqueueCommand");
			s_Renderer->GetRenderDevice()->EnqueueCommand<T>(resourceHandle, std::forward<Args>(args)...);
		}

		static void EnqueueResourceFree(CommandResourceHandle resourceHandle);
		static void EnqueueResourceFree(EnqueueFunc&& func);

		static void EnqueueToRenderThread(EnqueueFunc&& func);

		static void OnWindowResize();
		static void OnViewportResize();
		static Entity OnMousePicking(const Ref<GraphicsPipeline>& idPipeline);

		static void SetViewportArea(int32_t width, int32_t height);
		static void SetViewportMouse(float viewportMouseX, float viewportMouseY);

		inline static auto GetViewportArea() {
			struct Size { int32_t Width, Height; };
			return Size{ s_ViewportWidth, s_ViewportHeight };
		}

		inline static auto GetViewportMousePos() {
			struct Size { float Width, Height; };
			return Size{ s_ViewportMouseX, s_ViewportMouseY };
		}

		inline static const uint32_t GetCurrentImageIndex() { return s_Renderer->GetCurrentImageIndex(); }
		inline static const uint32_t GetCurrentFrameIndex() { return s_Renderer->GetCurrentFrameIndex(); }
		inline static const uint32_t GetMaxFramesInFlight() { return s_Renderer->GetMaxFramesInFlight(); }

		inline static const Ref<RenderContext>& GetRenderContext() { return s_Renderer->GetRenderContext(); }

		inline static RenderArchitecture GetRenderArchitecture() { return s_Architecture; }
	private:
		inline static Ref<RendererBase> s_Renderer = nullptr;
		inline static Ref<Scene> s_Scene = nullptr;

		inline static int32_t s_ViewportWidth = 0, s_ViewportHeight;
		inline static float s_ViewportMouseX = 0, s_ViewportMouseY;
		inline static RenderArchitecture s_Architecture;

		static void SetImGuiRenderData(std::function<void(VkCommandBuffer)>&& func);
		inline static std::function<void(VkCommandBuffer)> s_ImGuiRenderData;

		friend class ImGuiOverlay; //for m_RenderContext and SetImGuiRenderData
	};
}