#pragma once

#include "Renderer/Commands/CommandQueue.h"

namespace Lucy {

	class ComputePipeline;
	/*
	struct RenderDeviceResource {
		virtual void Destroy() = 0;
	};
	*/
	class RenderDevice {
	public:
		static Ref<RenderDevice> Create();

		virtual void Init();
		void Recreate();
		void EnqueueToRenderThread(EnqueueFunc&& func);

		/*
		virtual Ref<GraphicsPipeline> CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo) = 0;
		virtual Ref<ComputePipeline> CreateComputePipeline(const ComputePipelineCreateInfo& createInfo) = 0;
		virtual Ref<RenderPass> CreateRenderPass(const RenderPassCreateInfo& createInfo) = 0;
		virtual Ref<Image> CreateImage(const ImageCreateInfo& createInfo);
		virtual Ref<FrameBuffer> CreateFrameBuffer(const FrameBufferCreateInfo& createInfo) = 0;
		virtual Ref<VertexBuffer> CreateVertexBuffer(uint32_t size) = 0;
		virtual Ref<IndexBuffer> CreateIndexBuffer(uint32_t size) = 0;
		*/

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
		virtual void BindPushConstant(void* commandBufferHandle, Ref<ContextPipeline> pipeline, const VulkanPushConstant& pushConstant) = 0;
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

		virtual void BeginTimestamp(void* commandBufferHandle) = 0;
		virtual void EndTimestamp(void* commandBufferHandle) = 0;
		virtual double GetTimestampResults() = 0;

		virtual void Destroy();
	protected:
		std::vector<EnqueueFunc> m_RenderFunctionQueue;
		std::vector<EnqueueFunc> m_DeletionQueue;
		Ref<CommandQueue> m_CommandQueue = nullptr;
	};
}

