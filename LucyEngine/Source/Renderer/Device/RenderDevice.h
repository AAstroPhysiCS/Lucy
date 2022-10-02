#pragma once

#include "Renderer/Commands/CommandQueue.h"

namespace Lucy {

	class ComputePipeline;

	class RenderDevice {
	public:
		static Ref<RenderDevice> Create();

		void Init();
		void Recreate();
		void EnqueueToRenderThread(EnqueueFunc&& func);

		CommandResourceHandle CreateCommandResource(Ref<ContextPipeline> pipeline, CommandFunc&& func);
		CommandResourceHandle CreateChildCommandResource(CommandResourceHandle parentResourceHandle, Ref<GraphicsPipeline> childPipeline, CommandFunc&& func);

		/// <param name="currentFrameWaitSemaphore: image is available, image is renderable"></param>
		/// <param name="currentFrameSignalSemaphore: rendering finished, signal it"></param>
		void SubmitWorkToGPU(void* queueHandle, const Fence& currentFrameFence, const Semaphore& currentFrameWaitSemaphore, const Semaphore& currentFrameSignalSemaphore);

		template <typename Command, typename ... Args>
		inline void EnqueueCommand(CommandResourceHandle resourceHandle, Args&&... args) {
			LUCY_PROFILE_NEW_EVENT("RenderDevice::EnqueueCommand");
			m_CommandQueue->EnqueueCommand(resourceHandle, Memory::CreateRef<Command>(args...));
		}

		void EnqueueCommandResourceFree(CommandResourceHandle resourceHandle);
		void EnqueueResourceFree(EnqueueFunc&& func);

		void DispatchCommands();
		void ExecuteCommandQueue();

		//commandBufferHandle is the handle of the commandBuffer
		//Vulkan: VkCommandBuffer
		virtual void BindBuffers(void* commandBufferHandle, Ref<Mesh> mesh) = 0;
		virtual void BindPushConstant(void* commandBufferHandle, Ref<GraphicsPipeline> pipeline, const VulkanPushConstant& pushConstant) = 0;
		virtual void BindPipeline(void* commandBufferHandle, Ref<ContextPipeline> pipeline) = 0;
		virtual void UpdateDescriptorSets(Ref<ContextPipeline> pipeline) = 0;
		virtual void BindAllDescriptorSets(void* commandBufferHandle, Ref<ContextPipeline> pipeline) = 0;
		virtual void BindDescriptorSet(void* commandBufferHandle, Ref<ContextPipeline> pipeline, uint32_t setIndex) = 0;
		virtual void BindBuffers(void* commandBufferHandle, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer) = 0;
		virtual void DrawIndexed(void* commandBufferHandle, uint32_t indexCount, uint32_t instanceCount,
								 uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) = 0;
		virtual void DispatchCompute(void* commandBufferHandle, Ref<ComputePipeline> computePipeline, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;

		virtual void BeginRenderPass(void* commandBufferHandle, Ref<GraphicsPipeline> pipeline) = 0;
		virtual void EndRenderPass(Ref<GraphicsPipeline> pipeline) = 0;
		
		virtual void Destroy(); //leaving it to the child too
	protected:
		std::vector<EnqueueFunc> m_RenderFunctionQueue;
		std::vector<EnqueueFunc> m_DeletionQueue;
		Ref<CommandQueue> m_CommandQueue = nullptr;
	};
}

