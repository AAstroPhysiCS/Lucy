#pragma once

#include "Core/Base.h"
#include "Context/RenderContext.h"

#include "Renderer/Commands/DrawCommand.h"
#include "Renderer/Commands/CommandQueue.h"

#include "Scene/Entity.h"
#include "Shader/Shader.h"

namespace Lucy {

	class PushConstant;
	class DescriptorSet;

	//Vulkan error codes
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

	class RenderDevice {
	public:
		virtual ~RenderDevice() = default;
		
		static Ref<RenderDevice> Create(RenderArchitecture arch);

		void Dispatch();
		void ClearQueues();

		void Enqueue(EnqueueFunc&& func);
		void EnqueueStaticMesh(Priority priority, Ref<Mesh> mesh, const glm::mat4& entityTransform);
		
		//Parameter "func" are the individual passes, works in parallel
		void RecordStaticMeshToCommandQueue(Ref<Pipeline> pipeline, RecordFunc<VkCommandBuffer, Ref<DrawCommand>>&& func);

		virtual void Init() = 0;
		virtual void Destroy() = 0;

		virtual void BeginScene(Scene& scene) = 0;
		virtual void RenderScene() = 0;
		virtual PresentResult EndScene() = 0;
		virtual void Wait() = 0;
	public:
		virtual void BindBuffers(void* commandBufferHandle, Ref<Mesh> mesh) = 0;
		virtual void BindPushConstant(void* commandBufferHandle, Ref<Pipeline> pipeline, const PushConstant& pushConstant) = 0;
		virtual void BindPipeline(void* commandBufferHandle, Ref<Pipeline> pipeline) = 0; //for CommandQueue
		virtual void BindDescriptorSet(void* commandBufferHandle, Ref<Pipeline> pipeline, Ref<DescriptorSet> descriptorSet) = 0;
		//commandBufferHandle is the handle of the commandBuffer
		//Vulkan: VkCommandBuffer
		virtual void BindBuffers(void* commandBufferHandle, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer) = 0;
		virtual void DrawIndexed(void* commandBufferHandle, uint32_t indexCount, uint32_t instanceCount,
								uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) = 0;

		virtual void BeginRenderPass(void* commandBufferHandle, Ref<Pipeline> pipeline) = 0;
		virtual void EndRenderPass(Ref<Pipeline> pipeline) = 0;
	public:
		virtual Entity OnMousePicking() = 0;
		virtual void OnWindowResize() = 0;
		virtual void OnViewportResize() = 0;

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
	protected:
		RenderDevice(RenderArchitecture arch);

		mutable int32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
		mutable float m_ViewportMouseX = 0, m_ViewportMouseY = 0;

		std::vector<EnqueueFunc> m_RenderFunctionQueue;
		std::vector<Ref<DrawCommand>> m_StaticMeshDrawCommands;

		Ref<RenderContext> m_RenderContext;
		RenderArchitecture m_Architecture;

		inline static Ref<CommandQueue> s_CommandQueue = nullptr;

		friend class ImGuiOverlay; //for m_RenderContext
	};
}