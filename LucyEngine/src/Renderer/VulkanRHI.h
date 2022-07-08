#pragma once

#include "Context/RHI.h"

namespace Lucy {

	struct ImGuiPipeline;

	class VulkanRHI : public RHI {
	public:
		VulkanRHI(RenderArchitecture arch);
		virtual ~VulkanRHI() = default;

		void Init() override;
		void Dispatch() override;
		void Destroy() override;

		void BeginScene(Scene& scene) override;
		void RenderScene() override;
		PresentResult EndScene() override;

		void DirectCopyBuffer(VkBuffer& stagingBuffer, VkBuffer& buffer, VkDeviceSize size);
		void Enqueue(const SubmitFunc&& func) override;
		void EnqueueStaticMesh(Priority priority, Ref<Mesh> mesh, const glm::mat4& entityTransform) override;
		
		//Parameter "func" are the individual passes, works in parallel
		void RecordStaticMeshToCommandQueue(Ref<Pipeline> pipeline, RecordFunc<Ref<DrawCommand>>&& func) override;

		void BindBuffers(Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer) override;

		static void RecordSingleTimeCommand(std::function<void(VkCommandBuffer)>&& func);
		
		void OnWindowResize() override;
		void OnViewportResize() override;
		Entity OnMousePicking() override;

		void UIPass(const ImGuiPipeline& imguiPipeline, std::function<void(VkCommandBuffer commandBuffer)>&& imguiRenderFunc); //vulkan only
	};
}