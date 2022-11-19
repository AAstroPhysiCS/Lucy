#pragma once

#include "RenderDevice.h"

namespace Lucy {

	class VulkanRenderDevice : public RenderDevice {
	public:
		virtual void Init() final override;
		virtual void Destroy() final override;

		//commandBufferHandle is the handle of the commandBuffer
		//Vulkan: VkCommandBuffer
		void BindBuffers(void* commandBufferHandle, Ref<Mesh> mesh) final override;
		void BindPushConstant(void* commandBufferHandle, Ref<ContextPipeline> pipeline, const VulkanPushConstant& pushConstant) final override;
		void BindPipeline(void* commandBufferHandle, Ref<ContextPipeline> pipeline) final override;
		void UpdateDescriptorSets(Ref<ContextPipeline> pipeline) final override;
		void BindAllDescriptorSets(void* commandBufferHandle, Ref<ContextPipeline> pipeline) final override;
		void BindDescriptorSet(void* commandBufferHandle, Ref<ContextPipeline> pipeline, uint32_t setIndex) final override;
		void BindBuffers(void* commandBufferHandle, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer) final override;
		void DrawIndexed(void* commandBufferHandle, uint32_t indexCount, uint32_t instanceCount,
						 uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) final override;
		void DispatchCompute(void* commandBufferHandle, Ref<ComputePipeline> computePipeline, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) final override;
		
		void BeginRenderPass(void* commandBufferHandle, Ref<GraphicsPipeline> pipeline) final override;
		void EndRenderPass(Ref<GraphicsPipeline> pipeline) final override;

		void BeginTimestamp(void* commandBufferHandle) final override;
		void EndTimestamp(void* commandBufferHandle) final override;
		double GetTimestampResults() final override;

		void SubmitImmediateCommand(std::function<void(VkCommandBuffer)>&& func);
	private:
		inline static constexpr uint32_t s_QueryCount = 64;
		std::stack<uint32_t> m_QueryStack;
		std::vector<VkQueryPool> m_QueryPools;
	};
}