#pragma once

#include <filesystem>

#include "RenderDeviceResourceManager.h"
#include "../RendererConfiguration.h"

#include "Renderer/Semaphore.h"

#include "RenderDeviceQueries.h"

namespace Lucy {

	struct GraphicsPipelineCreateInfo;
	struct ComputePipelineCreateInfo;

	struct RenderPassCreateInfo;
	struct FrameBufferCreateInfo;

	struct ImageCreateInfo;
	struct ImageSamplerCreateInfo;

	struct DescriptorSetCreateInfo;
	struct SharedStorageBufferCreateInfo;
	struct UniformBufferCreateInfo;

	class FrameBuffer;
	class VertexBuffer;
	class IndexBuffer;

	class Image;
	class VulkanImage2D;

	class RenderDeviceBuffer;
	class RenderDeviceScene;
	class RenderPass;

	class Shader;
	class Mesh;

	struct ExecutionBatch;

	class Pipeline;
	class PipelineConstant;
	class PipelineManager;
	class GraphicsPipeline;
	class ComputePipeline;

	class RenderCommandQueue;
	class RenderCommandList;
	class CommandPool;

	enum class TargetQueueFamily : uint8_t {
		Graphics,
		Compute,
		Transfer,
		Count
	};

	class RenderDevice : public MemoryTrackable {
	public:
		static Ref<RenderDevice> Create(RendererConfiguration config);
	public:
		RenderDevice() = default;
		virtual ~RenderDevice() = default;

		RenderDevice(const RenderDevice&) = delete;
		RenderDevice& operator=(const RenderDevice&) = delete;
		RenderDevice(RenderDevice&&) = delete;
		RenderDevice& operator=(RenderDevice&&) = delete;
	public:
		[[nodiscard]] virtual uint32_t BindGlobalImageHandleTo(const std::string& imageBufferName, const Ref<GraphicsPipeline>& pipeline, const Ref<Image>& image, uint32_t mip) = 0;
		[[nodiscard]] virtual uint32_t BindGlobalImageHandleTo(const std::string& imageBufferName, const Ref<ComputePipeline>& pipeline, const Ref<Image>& image, uint32_t mip) = 0;

		[[nodiscard]] const Unique<RenderDeviceScene>& GetScene() { return m_DeviceScene; }
#pragma region ResourceManager
		[[nodiscard]] RenderDeviceResourceHandle CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo, const Ref<Shader>& shader);
		[[nodiscard]] RenderDeviceResourceHandle CreateComputePipeline(const ComputePipelineCreateInfo& createInfo, const Ref<Shader>& shader);
		[[nodiscard]] RenderDeviceResourceHandle CreateRenderPass(const RenderPassCreateInfo& createInfo);

		[[nodiscard]] RenderDeviceResourceHandle CreateFrameBuffer(const FrameBufferCreateInfo& createInfo);
		[[nodiscard]] RenderDeviceResourceHandle CreateVertexBuffer(size_t size);
		[[nodiscard]] RenderDeviceResourceHandle CreateIndexBuffer(size_t size);
		[[nodiscard]] RenderDeviceResourceHandle CreateDeviceAddressBuffer(const RenderDeviceBufferCreateInfo& createInfo);

		[[nodiscard]] RenderDeviceResourceHandle CreateDescriptorSet(const DescriptorSetCreateInfo& createInfo);
		[[nodiscard]] RenderDeviceResourceHandle CreateSampler(const ImageSamplerCreateInfo& createInfo);
		[[nodiscard]] RenderDeviceResourceHandle CreateSharedStorageBuffer(const SharedStorageBufferCreateInfo& createInfo);
		[[nodiscard]] RenderDeviceResourceHandle CreateUniformBuffer(const UniformBufferCreateInfo& createInfo);

		[[nodiscard]] RenderDeviceResourceHandle CreateImage(const std::filesystem::path& path, ImageCreateInfo& createInfo, std::string_view debugName = {});
		[[nodiscard]] RenderDeviceResourceHandle CreateImage(const ImageCreateInfo& createInfo, std::string_view debugName = {});
		[[nodiscard]] RenderDeviceResourceHandle CreateImage(const Ref<VulkanImage2D>& other);
		
		template <typename TResource> requires IsRenderResource<TResource>
		[[nodiscard]] inline Ref<TResource> AccessResource(RenderDeviceResourceHandle handle) {
			return m_ResourceManager.GetResource(handle)->As<TResource>();
		}
		[[nodiscard]] bool IsValidResource(RenderDeviceResourceHandle handle) const;
		void RTDestroyResource(RenderDeviceResourceHandle& handle);
#pragma endregion ResourceManager
		void CreatePipelineDeviceQueries(size_t pipelineCount);
		void CreateTimestampDeviceQueries(size_t passCount);

		[[nodiscard]] uint32_t RTBeginTimestamp(Ref<CommandPool> cmdPool);
		[[nodiscard]] uint32_t RTEndTimestamp(Ref<CommandPool> cmdPool);
		void ResetTimestampQuery(uint32_t frameIndex);
		void RTResetTimestampQuery(Ref<CommandPool> commandPool);

		[[nodiscard]] uint32_t RTBeginPipelineQuery(Ref<CommandPool> cmdPool);
		[[nodiscard]] uint32_t RTEndPipelineQuery(Ref<CommandPool> cmdPool);
		void ResetPipelineQuery(uint32_t frameIndex);
		void RTResetPipelineQuery(Ref<CommandPool> commandPool);

		std::vector<uint64_t> GetQueryResults(RenderDeviceQueryType type, uint32_t frameIndex);

		virtual void RegisterShaderBindings(const Ref<Shader>& shader) = 0;
		virtual std::vector<RenderDeviceResourceHandle> GetResourceBindingHandles(const Ref<Shader>& shader) const = 0;

		virtual void SubmitWorkToGPUAsBatch(const RenderCommandList& renderCommandList, const ExecutionBatch& batch) = 0;

		virtual void WaitForDevice() = 0;
		virtual void WaitForQueue(TargetQueueFamily queueFamily) = 0;

		virtual void BeginCommandBuffer(Ref<CommandPool> cmdPool) = 0;
		virtual void EndCommandBuffer(Ref<CommandPool> cmdPool) = 0;

		virtual void FillBuffer(Ref<CommandPool> cmdPool, Ref<RenderDeviceBuffer> buffer, size_t offset, size_t size, uint32_t value) = 0;
		virtual void CopyBuffer(Ref<CommandPool> cmdPool, Ref<RenderDeviceBuffer> srcBuffer, Ref<RenderDeviceBuffer> dstBuffer, size_t srcOffset, size_t dstOffset, size_t size) = 0;
		virtual void CopyBuffer(Ref<CommandPool> cmdPool, Ref<RenderDeviceBuffer> srcBuffer, Ref<RenderDeviceBuffer> dstBuffer, const std::vector<const void*>& regions) = 0;

		virtual void BindBuffers(Ref<CommandPool> cmdPool, Ref<RenderDeviceBuffer> indexBuffer) = 0;
		virtual void BindBuffers(Ref<CommandPool> cmdPool, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer) = 0;

		virtual void BindPushConstant(Ref<CommandPool> cmdPool, Ref<GraphicsPipeline> pipeline, const PipelineConstant& pushConstant) = 0;
		virtual void BindPushConstant(Ref<CommandPool> cmdPool, Ref<ComputePipeline> pipeline, const PipelineConstant& pushConstant) = 0;

		virtual void BindPipeline(Ref<CommandPool> cmdPool, Ref<GraphicsPipeline> pipeline) = 0;
		virtual void BindPipeline(Ref<CommandPool> cmdPool, Ref<ComputePipeline> pipeline) = 0;
		
		virtual void UpdateDescriptorSets(Ref<GraphicsPipeline> pipeline) = 0;
		virtual void UpdateDescriptorSets(Ref<ComputePipeline> pipeline) = 0;

		virtual void BindAllDescriptorSets(Ref<CommandPool> cmdPool, Ref<GraphicsPipeline> pipeline) = 0;
		virtual void BindDescriptorSet(Ref<CommandPool> cmdPool, Ref<GraphicsPipeline> pipeline, uint32_t setIndex) = 0;
		
		virtual void BindAllDescriptorSets(Ref<CommandPool> cmdPool, Ref<ComputePipeline> pipeline) = 0;
		virtual void BindDescriptorSet(Ref<CommandPool> cmdPool, Ref<ComputePipeline> pipeline, uint32_t setIndex) = 0;

		virtual void DrawIndexedIndirectCount(Ref<CommandPool> cmdPool, Ref<RenderDeviceBuffer> buffer, size_t offset,
			Ref<RenderDeviceBuffer> countBuffer, size_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride) = 0;
		virtual void DrawIndexed(Ref<CommandPool> cmdPool, uint32_t indexCount, uint32_t instanceCount, 
			uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) = 0;

		virtual void DispatchCompute(Ref<CommandPool> cmdPool, Ref<ComputePipeline> computePipeline, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;
		virtual void DispatchComputeIndirect(Ref<CommandPool> cmdPool, Ref<RenderDeviceBuffer> buffer, size_t offset) = 0;

		virtual void BeginRenderPass(Ref<RenderPass> renderPass, Ref<FrameBuffer> frameBuffer, Ref<CommandPool> cmdPool) = 0;
		virtual void EndRenderPass(Ref<RenderPass> renderPass) = 0;

		virtual void BeginDebugMarker(Ref<CommandPool> cmdPool, const char* labelName) = 0;
		virtual void EndDebugMarker(Ref<CommandPool> cmdPool) = 0;

		virtual void Destroy() = 0;
	private:
		RenderDeviceResourceManager m_ResourceManager{ this };
	protected:
		Unique<RenderDeviceScene> m_DeviceScene = nullptr;

		Ref<RenderDeviceQuery> m_RenderDeviceTimestampQuery = nullptr; //initialized after we call CreateDeviceQueries
		Ref<RenderDeviceQuery> m_RenderDevicePipelineQuery = nullptr;
	};
}

