#pragma once

#include <filesystem>

#include "RenderDeviceResourceManager.h"

#include "Renderer/Synchronization/VulkanSyncItems.h"
#include "RenderDeviceQueries.h"

namespace Lucy {

	struct GraphicsPipelineCreateInfo;
	struct ComputePipelineCreateInfo;
	struct RenderPassCreateInfo;
	struct FrameBufferCreateInfo;
	struct ImageCreateInfo;

	struct DescriptorSetCreateInfo;
	struct SharedStorageBufferCreateInfo;
	struct UniformBufferCreateInfo;

	class FrameBuffer;
	class VertexBuffer;
	class IndexBuffer;
	class VulkanImage2D;

	class RenderPass;

	class Mesh;

	class GraphicsPipeline;
	class ComputePipeline;

	class RenderCommandQueue;
	class CommandPool;

	class VulkanPushConstant;

	class PipelineManager;

	enum class TargetQueueFamily : uint8_t {
		Graphics,
		Compute,
		Transfer,
	};

	class RenderDevice : public MemoryTrackable {
	public:
		static Ref<RenderDevice> Create(RendererConfiguration config);
	public:
		RenderDevice() = default;
		virtual ~RenderDevice() = default;
#pragma region ResourceManager
		RenderResourceHandle CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo);
		RenderResourceHandle CreateComputePipeline(const ComputePipelineCreateInfo& createInfo);
		RenderResourceHandle CreateRenderPass(const RenderPassCreateInfo& createInfo);

		RenderResourceHandle CreateFrameBuffer(const FrameBufferCreateInfo& createInfo);
		RenderResourceHandle CreateVertexBuffer(size_t size);
		RenderResourceHandle CreateIndexBuffer(size_t size);

		RenderResourceHandle CreateDescriptorSet(const DescriptorSetCreateInfo& createInfo);
		RenderResourceHandle CreateSharedStorageBuffer(const SharedStorageBufferCreateInfo& createInfo);
		RenderResourceHandle CreateUniformBuffer(const UniformBufferCreateInfo& createInfo);

		RenderResourceHandle CreateImage(const std::filesystem::path& path, ImageCreateInfo& createInfo);
		RenderResourceHandle CreateImage(const ImageCreateInfo& createInfo);
		RenderResourceHandle CreateImage(const Ref<VulkanImage2D>& other);

		template <typename TResource> requires IsRenderResource<TResource>
		inline Ref<TResource> AccessResource(RenderResourceHandle handle) {
			return m_ResourceManager.GetResource(handle)->As<TResource>();
		}
		bool IsValidResource(RenderResourceHandle handle) const;
		void RTDestroyResource(RenderResourceHandle& handle);
#pragma endregion ResourceManager
		void CreatePipelineDeviceQueries(size_t pipelineCount);
		void CreateTimestampDeviceQueries(size_t passCount);

		uint32_t RTBeginTimestamp(Ref<CommandPool> cmdPool);
		uint32_t RTEndTimestamp(Ref<CommandPool> cmdPool);
		void RTResetTimestampQuery(Ref<CommandPool> commandPool);

		uint32_t RTBeginPipelineQuery(Ref<CommandPool> cmdPool);
		uint32_t RTEndPipelineQuery(Ref<CommandPool> cmdPool);
		void RTResetPipelineQuery(Ref<CommandPool> commandPool);

		std::vector<uint64_t> GetQueryResults(RenderDeviceQueryType type);

		/// <param name="currentFrameWaitSemaphore: image is available, image is renderable"></param>
		/// <param name="currentFrameSignalSemaphore: rendering finished, signal it"></param>
		virtual void SubmitWorkToGPU(TargetQueueFamily queueFamily, Ref<CommandPool> cmdPool,
			Fence* currentFrameFence, Semaphore* currentFrameWaitSemaphore, Semaphore* currentFrameSignalSemaphore) = 0;
		virtual bool SubmitWorkToGPU(TargetQueueFamily queueFamily, std::vector<Ref<CommandPool>>& cmdPools,
			Fence* currentFrameFence, Semaphore* currentFrameWaitSemaphore, Semaphore* currentFrameSignalSemaphore) = 0;
		virtual void SubmitWorkToGPU(TargetQueueFamily queueFamily, std::vector<Ref<CommandPool>>& cmdPools,
			Fence* currentFrameFence, Semaphore* currentFrameWaitSemaphore) = 0;

		virtual void WaitForDevice() = 0;
		virtual void WaitForQueue(TargetQueueFamily queueFamily) = 0;

		virtual void BeginCommandBuffer(Ref<CommandPool> cmdPool) = 0;
		virtual void EndCommandBuffer(Ref<CommandPool> cmdPool) = 0;

		virtual void BindBuffers(Ref<CommandPool> cmdPool, Ref<Mesh> mesh) = 0;
		virtual void BindBuffers(Ref<CommandPool> cmdPool, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer) = 0;

		virtual void BindPushConstant(Ref<CommandPool> cmdPool, Ref<GraphicsPipeline> pipeline, const VulkanPushConstant& pushConstant) = 0;
		virtual void BindPushConstant(Ref<CommandPool> cmdPool, Ref<ComputePipeline> pipeline, const VulkanPushConstant& pushConstant) = 0;

		virtual void BindPipeline(Ref<CommandPool> cmdPool, Ref<GraphicsPipeline> pipeline) = 0;
		virtual void BindPipeline(Ref<CommandPool> cmdPool, Ref<ComputePipeline> pipeline) = 0;
		
		virtual void UpdateDescriptorSets(Ref<GraphicsPipeline> pipeline) = 0;
		virtual void UpdateDescriptorSets(Ref<ComputePipeline> pipeline) = 0;

		virtual void BindAllDescriptorSets(Ref<CommandPool> cmdPool, Ref<GraphicsPipeline> pipeline) = 0;
		virtual void BindDescriptorSet(Ref<CommandPool> cmdPool, Ref<GraphicsPipeline> pipeline, uint32_t setIndex) = 0;
		
		virtual void BindAllDescriptorSets(Ref<CommandPool> cmdPool, Ref<ComputePipeline> pipeline) = 0;
		virtual void BindDescriptorSet(Ref<CommandPool> cmdPool, Ref<ComputePipeline> pipeline, uint32_t setIndex) = 0;

		virtual void DrawIndexed(Ref<CommandPool> cmdPool, uint32_t indexCount, uint32_t instanceCount,
								 uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) = 0;
		virtual void DispatchCompute(Ref<CommandPool> cmdPool, Ref<ComputePipeline> computePipeline, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;

		virtual void BeginRenderPass(Ref<RenderPass> renderPass, Ref<FrameBuffer> frameBuffer, Ref<CommandPool> cmdPool) = 0;
		virtual void EndRenderPass(Ref<RenderPass> renderPass) = 0;

		virtual void BeginDebugMarker(Ref<CommandPool> cmdPool, const char* labelName) = 0;
		virtual void EndDebugMarker(Ref<CommandPool> cmdPool) = 0;

		virtual void Destroy() = 0;
	private:
		RenderDeviceResourceManager m_ResourceManager;
	protected:
		Ref<RenderDeviceQuery> m_RenderDeviceTimestampQuery = nullptr; //initialized after we call CreateDeviceQueries
		Ref<RenderDeviceQuery> m_RenderDevicePipelineQuery = nullptr;
	};
}

