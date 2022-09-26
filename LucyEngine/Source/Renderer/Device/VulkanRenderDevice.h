#pragma once

#include "RenderDevice.h"

namespace Lucy {

	class VulkanRenderDevice : public RenderDevice {
	public:
		//commandBufferHandle is the handle of the commandBuffer
		//Vulkan: VkCommandBuffer
		void BindBuffers(void* commandBufferHandle, Ref<Mesh> mesh) final override;
		void BindPushConstant(void* commandBufferHandle, Ref<GraphicsPipeline> pipeline, const VulkanPushConstant& pushConstant) final override;
		void BindPipeline(void* commandBufferHandle, Ref<GraphicsPipeline> pipeline) final override;
		void UpdateDescriptorSets(Ref<GraphicsPipeline> pipeline) final override;
		void BindAllDescriptorSets(void* commandBufferHandle, Ref<GraphicsPipeline> pipeline) final override;
		void BindDescriptorSet(void* commandBufferHandle, Ref<GraphicsPipeline> pipeline, uint32_t setIndex) final override;
		void BindBuffers(void* commandBufferHandle, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer) final override;
		void DrawIndexed(void* commandBufferHandle, uint32_t indexCount, uint32_t instanceCount,
						 uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) final override;
		
		void BeginRenderPass(void* commandBufferHandle, Ref<GraphicsPipeline> pipeline) final override;
		void EndRenderPass(Ref<GraphicsPipeline> pipeline) final override;

		void SubmitImmediateCommand(std::function<void(VkCommandBuffer)>&& func);
	};
}