#pragma once

#include "Context/RHI.h"
#include "Context/VulkanContext.h"

#include "Synchronization/SynchItems.h"
#include "VulkanRenderPass.h"

namespace Lucy {

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
		void EnqueueStaticMesh(RefLucy<Mesh> mesh, const glm::mat4& entityTransform) override;
		
		//Parameter "func" are the individual passes, works in parallel
		void RecordToCommandQueue(RecordFunc<>&& func) override;
		void RecordToCommandQueue(RecordFunc<MeshDrawCommand>&& func) override;

		void BindPipeline(RefLucy<Pipeline> pipeline) override;
		void UnbindPipeline(RefLucy<Pipeline> pipeline) override;
		void BindBuffers(RefLucy<VertexBuffer> vertexBuffer, RefLucy<IndexBuffer> indexBuffer) override;

		static void RecordSingleTimeCommand(std::function<void(VkCommandBuffer)>&& func);
		
		void OnViewportResize() override;
		Entity OnMousePicking() override;
	};
}