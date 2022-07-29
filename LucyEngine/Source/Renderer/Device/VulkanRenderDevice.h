#pragma once

#include "RenderDevice.h"

namespace Lucy {

	class VulkanRenderDevice : public RenderDevice {
	public:
		//commandBufferHandle is the handle of the commandBuffer
		//Vulkan: VkCommandBuffer
		void BindBuffers(void* commandBufferHandle, Ref<Mesh> mesh) final override;
		void BindPushConstant(void* commandBufferHandle, Ref<Pipeline> pipeline, const PushConstant& pushConstant) final override;
		void BindPipeline(void* commandBufferHandle, Ref<Pipeline> pipeline) final override;
		void UpdateDescriptorSets(Ref<Pipeline> pipeline) final override;
		void BindAllDescriptorSets(void* commandBufferHandle, Ref<Pipeline> pipeline) final override;
		void BindDescriptorSet(void* commandBufferHandle, Ref<Pipeline> pipeline, uint32_t setIndex) final override;
		void BindBuffers(void* commandBufferHandle, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer) final override;
		void DrawIndexed(void* commandBufferHandle, uint32_t indexCount, uint32_t instanceCount,
						 uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) final override;

		void BeginRenderPass(void* commandBufferHandle, Ref<Pipeline> pipeline) final override;
		void EndRenderPass(Ref<Pipeline> pipeline) final override;

		void ExecuteSingleTimeCommand(std::function<void(VkCommandBuffer)>&& func);
	};
}