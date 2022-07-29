#pragma once

#include "RenderDeviceCommandList.h"

namespace Lucy {

	class RenderDevice {
	public:
		static Ref<RenderDevice> Create();

		void Init();
		void Recreate();
		void EnqueueToRenderThread(EnqueueFunc&& func);
		RenderCommandResourceHandle CreateRenderPassResource(RenderCommandFunc&& func, Ref<Pipeline> pipeline);

		template <typename Command, typename ... Args>
		inline void EnqueueRenderCommand(RenderCommandResourceHandle resourceHandle, Args&&... args) {
			m_RenderDeviceCommandList->EnqueueRenderCommand<Command>(resourceHandle, std::forward<Args>(args)...);
		}

		void DispatchCommands();
		void ExecuteCommandQueue();

		//commandBufferHandle is the handle of the commandBuffer
		//Vulkan: VkCommandBuffer
		virtual void BindBuffers(void* commandBufferHandle, Ref<Mesh> mesh) = 0;
		virtual void BindPushConstant(void* commandBufferHandle, Ref<Pipeline> pipeline, const PushConstant& pushConstant) = 0;
		virtual void BindPipeline(void* commandBufferHandle, Ref<Pipeline> pipeline) = 0;
		virtual void UpdateDescriptorSets(Ref<Pipeline> pipeline) = 0;
		virtual void BindAllDescriptorSets(void* commandBufferHandle, Ref<Pipeline> pipeline) = 0;
		virtual void BindDescriptorSet(void* commandBufferHandle, Ref<Pipeline> pipeline, uint32_t setIndex) = 0;
		virtual void BindBuffers(void* commandBufferHandle, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer) = 0;
		virtual void DrawIndexed(void* commandBufferHandle, uint32_t indexCount, uint32_t instanceCount,
								 uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) = 0;

		virtual void BeginRenderPass(void* commandBufferHandle, Ref<Pipeline> pipeline) = 0;
		virtual void EndRenderPass(Ref<Pipeline> pipeline) = 0;
		
		virtual void Destroy(); //leaving it to the child too
	private:
		inline const Ref<RenderDeviceCommandList>& GetRenderDeviceCommandList() const { return m_RenderDeviceCommandList; }
		//only specific classes should have access to command list
		friend class VulkanRenderer;
	protected:
		Ref<RenderDeviceCommandList> m_RenderDeviceCommandList = nullptr;
	};
}

