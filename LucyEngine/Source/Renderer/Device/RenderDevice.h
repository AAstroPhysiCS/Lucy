#pragma once

#include "Renderer/Commands/CommandQueue.h"

namespace Lucy {

	class RenderDevice {
	public:
		static Ref<RenderDevice> Create();

		void Init();
		void Recreate();
		void EnqueueToRenderThread(EnqueueFunc&& func);
		CommandResourceHandle CreateCommandResource(CommandFunc&& func, Ref<GraphicsPipeline> pipeline);

		template <typename Command, typename ... Args>
		inline void EnqueueCommand(CommandResourceHandle resourceHandle, Args&&... args) {
			LUCY_PROFILE_NEW_EVENT("RenderDevice::EnqueueCommand");
			m_CommandQueue->EnqueueCommand(resourceHandle, Memory::CreateRef<Command>(args...));
		}

		void EnqueueResourceFree(CommandResourceHandle resourceHandle);
		void EnqueueResourceFree(EnqueueFunc&& func);

		void DispatchCommands();
		void ExecuteCommandQueue();

		//commandBufferHandle is the handle of the commandBuffer
		//Vulkan: VkCommandBuffer
		virtual void BindBuffers(void* commandBufferHandle, Ref<Mesh> mesh) = 0;
		virtual void BindPushConstant(void* commandBufferHandle, Ref<GraphicsPipeline> pipeline, const VulkanPushConstant& pushConstant) = 0;
		virtual void BindPipeline(void* commandBufferHandle, Ref<GraphicsPipeline> pipeline) = 0;
		virtual void UpdateDescriptorSets(Ref<GraphicsPipeline> pipeline) = 0;
		virtual void BindAllDescriptorSets(void* commandBufferHandle, Ref<GraphicsPipeline> pipeline) = 0;
		virtual void BindDescriptorSet(void* commandBufferHandle, Ref<GraphicsPipeline> pipeline, uint32_t setIndex) = 0;
		virtual void BindBuffers(void* commandBufferHandle, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer) = 0;
		virtual void DrawIndexed(void* commandBufferHandle, uint32_t indexCount, uint32_t instanceCount,
								 uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) = 0;

		virtual void BeginRenderPass(void* commandBufferHandle, Ref<GraphicsPipeline> pipeline) = 0;
		virtual void EndRenderPass(Ref<GraphicsPipeline> pipeline) = 0;
		
		virtual void Destroy(); //leaving it to the child too

		friend class VulkanRenderer;
	protected:
		std::vector<EnqueueFunc> m_RenderFunctionQueue;
		std::vector<EnqueueFunc> m_DeletionQueue;
		Ref<CommandQueue> m_CommandQueue = nullptr;
	};
}

