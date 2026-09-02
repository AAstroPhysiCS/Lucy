#pragma once

#include "RenderDevice.h"
#include "Renderer/Memory/VulkanAllocator.h"
#include "Renderer/Memory/VulkanRenderDeviceUploadManager.h"

#include "Renderer/Memory/Buffer/PushConstant.h"

#include "Renderer/Semaphore.h"
#include "Renderer/ExecutionBatch.h"

#include "Renderer/Descriptors/DescriptorSetManager.h"

namespace Lucy {

	class Mesh;

	struct VulkanDeviceInfo {
		std::string Name;

		uint32_t VendorId;
		uint32_t DeviceId;
		VkPhysicalDeviceType DeviceType;

		uint32_t DriverVersion;
		uint32_t ApiVersion;

		uint32_t MinUniformBufferAlignment;
		uint32_t MinStorageBufferAlignment;
		uint32_t MinTexelBufferOffsetAlignment;

		uint32_t MaxImageDimension2D;
		uint32_t MaxBoundDescriptorSets;
		uint32_t MaxPushConstantsSize;

		uint32_t MaxComputeWorkGroupInvocations;
		uint32_t MaxComputeWorkGroupSize[3];
		uint32_t MaxComputeWorkGroupCount[3];

		float MaxSamplerAnisotropy;
		float TimestampPeriod;

		VkSampleCountFlags FramebufferColorSampleCounts;
		VkSampleCountFlags FramebufferDepthSampleCounts;

		bool SamplerAnisotropy;
		bool GeometryShader;
		bool TessellationShader;
		bool MultiViewport;

		bool DynamicRendering;
		bool Synchronization2;
		bool TimelineSemaphore;
		bool BufferDeviceAddress;
		bool DescriptorIndexing;

		bool RayTracingPipeline;
		bool AccelerationStructure;

		uint32_t ShaderGroupHandleSize;
		uint32_t ShaderGroupHandleAlignment;
		uint32_t ShaderGroupBaseAlignment;
		uint32_t MaxShaderGroupStride;
		uint32_t MaxRayRecursionDepth;
	};

	struct QueueFamilyIndices {
		uint32_t GraphicsFamily = UINT32_MAX;
		uint32_t PresentFamily = UINT32_MAX;
		uint32_t ComputeFamily = UINT32_MAX;
		uint32_t TransferFamily = UINT32_MAX;

		bool GraphicsFamilyHasValue = false;
		bool PresentFamilyHasValue = false;
		bool ComputeFamilyHasValue = false;
		bool TransferFamilyHasValue = false;

		bool IsComplete() const { return GraphicsFamilyHasValue && PresentFamilyHasValue && ComputeFamilyHasValue && TransferFamilyHasValue; }
	};

	class VulkanRenderDevice : public RenderDevice {
	public:
		VulkanRenderDevice() = default;
		virtual ~VulkanRenderDevice() = default;

		VulkanRenderDevice(const VulkanRenderDevice&) = delete;
		VulkanRenderDevice& operator=(const VulkanRenderDevice&) = delete;
		VulkanRenderDevice(VulkanRenderDevice&&) = delete;
		VulkanRenderDevice& operator=(VulkanRenderDevice&&) = delete;

		void Init(VkInstance instance, const std::vector<const char*>& enabledValidationLayers, VkSurfaceKHR surface, uint32_t apiVersion);
		void CreateDeviceResources() final override;
		void Destroy() final override;

		void BeginCommandBuffer(Ref<CommandPool> cmdPool);
		void EndCommandBuffer(Ref<CommandPool> cmdPool);

		void FillBuffer(Ref<CommandPool> cmdPool, Ref<RenderDeviceBuffer> buffer, size_t offset, size_t size, uint32_t value) final override;
		void CopyBuffer(Ref<CommandPool> cmdPool, Ref<RenderDeviceBuffer> srcBuffer, Ref<RenderDeviceBuffer> dstBuffer, size_t srcOffset, size_t dstOffset, size_t size) final override;
		void CopyBuffer(Ref<CommandPool> cmdPool, Ref<RenderDeviceBuffer> srcBuffer, Ref<RenderDeviceBuffer> dstBuffer, const std::vector<const void*>& regions) final override;

		void BindBuffers(Ref<CommandPool> cmdPool, Ref<RenderDeviceBuffer> indexBuffer) final override;
		void BindBuffers(Ref<CommandPool> cmdPool, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer) final override;

		[[nodiscard]] RenderDeviceTextureHandle BindGlobalImageHandleTo(const std::string& imageBufferName, const Ref<GraphicsPipeline>& pipeline, const Ref<Image>& image, uint32_t mip) final override;
		[[nodiscard]] RenderDeviceTextureHandle BindGlobalImageHandleTo(const std::string& imageBufferName, const Ref<ComputePipeline>& pipeline, const Ref<Image>& image, uint32_t mip) final override;

		void BindPushConstant(Ref<CommandPool> cmdPool, Ref<GraphicsPipeline> pipeline, const PipelineConstant& pushConstant) final override;
		void BindPushConstant(Ref<CommandPool> cmdPool, Ref<ComputePipeline> pipeline, const PipelineConstant& pushConstant) final override;
		void BindPushConstant(Ref<CommandPool> cmdPool, Ref<RayTracingPipeline> pipeline, const PipelineConstant& pushConstant) final override;

		void BindPipeline(Ref<CommandPool> cmdPool, Ref<GraphicsPipeline> pipeline) final override;
		void BindPipeline(Ref<CommandPool> cmdPool, Ref<ComputePipeline> pipeline) final override;
		void BindPipeline(Ref<CommandPool> cmdPool, Ref<RayTracingPipeline> pipeline) final override;

		void TraceRays(Ref<CommandPool> cmdPool, Ref<RayTracingPipeline> pipeline, uint32_t width, uint32_t height, uint32_t depth) final override;

		void UpdateDescriptorSets(Ref<GraphicsPipeline> pipeline) final override;
		void UpdateDescriptorSets(Ref<ComputePipeline> pipeline) final override;
		void UpdateDescriptorSets(Ref<RayTracingPipeline> pipeline, const std::string& name, const Ref<AccelerationStructure>& accelerationStructure) final override;

		void BindAllDescriptorSets(Ref<CommandPool> cmdPool, Ref<GraphicsPipeline> pipeline) final override;
		void BindDescriptorSet(Ref<CommandPool> cmdPool, Ref<GraphicsPipeline> pipeline, uint32_t setIndex) final override;

		void BindAllDescriptorSets(Ref<CommandPool> cmdPool, Ref<ComputePipeline> pipeline) final override;
		void BindDescriptorSet(Ref<CommandPool> cmdPool, Ref<ComputePipeline> pipeline, uint32_t setIndex) final override;

		void BindAllDescriptorSets(Ref<CommandPool> cmdPool, Ref<RayTracingPipeline> pipeline) final override;
		void BindDescriptorSet(Ref<CommandPool> cmdPool, Ref<RayTracingPipeline> pipeline, uint32_t setIndex) final override;

		void DrawIndexedIndirectCount(Ref<CommandPool> cmdPool, Ref<RenderDeviceBuffer> buffer, size_t offset, 
			Ref<RenderDeviceBuffer> countBuffer, size_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride) final override;
		void DrawIndexed(Ref<CommandPool> cmdPool, uint32_t indexCount, uint32_t instanceCount,
						 uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) final override;

		void DispatchCompute(Ref<CommandPool> cmdPool, Ref<ComputePipeline> computePipeline, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) final override;
		void DispatchComputeIndirect(Ref<CommandPool> cmdPool, Ref<RenderDeviceBuffer> buffer, size_t offset) final override;

		void BeginRenderPass(Ref<RenderPass> renderPass, Ref<FrameBuffer> frameBuffer, Ref<CommandPool> cmdPool) final override;
		void EndRenderPass(Ref<RenderPass> renderPass) final override;

		void BeginDebugMarker(Ref<CommandPool> cmdPool, const char* labelName) final override;
		void BeginDebugMarker(VkCommandBuffer commandBuffer, const char* labelName);
		void EndDebugMarker(Ref<CommandPool> cmdPool) final override;
		void EndDebugMarker(VkCommandBuffer commandBuffer);

		void RegisterShaderBindings(const Ref<Shader>& shader) final override;

		void SubmitWorkToGPU(const RenderCommandList& renderCommandList, const std::vector<VulkanQueueSubmitInfo>& waits,
			VulkanSemaphore& renderFinishedSemaphore, VulkanSemaphore& frameTimelineSemaphore, uint64_t signalValue);
		void SubmitWorkToGPUAsBatch(const RenderCommandList& renderCommandList, const ExecutionBatch& batch) final override;

		void SubmitImmediateCommand(const std::function<void(VkCommandBuffer)>& func);

		void WaitForDevice() final override;
		void WaitForQueue(TargetQueueFamily queueFamily) final override;

		VulkanDeviceInfo& GetDeviceInformation() { return m_DeviceInfo; }

		std::vector<RenderDeviceResourceHandle> GetResourceBindingHandles(const Ref<Shader>& shader) const final override;

		VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
		VkDevice GetLogicalDevice() const { return m_LogicalDevice; }
		QueueFamilyIndices GetQueueFamilies() const { return m_QueueFamilyIndices; }

		auto GetQueue(TargetQueueFamily queueFamily) {
			struct Result {
				uint32_t Family;
				VkQueue Handle;
			};

			switch (queueFamily) {
				case TargetQueueFamily::Graphics: return Result { GetQueueFamilies().GraphicsFamily, m_GraphicsQueue };
				case TargetQueueFamily::Compute:  return Result { GetQueueFamilies().ComputeFamily, m_ComputeQueue };
				case TargetQueueFamily::Transfer: return Result { GetQueueFamilies().TransferFamily, m_TransferQueue };
				default: LUCY_ASSERT(false);
			}
		}

		const Unique<VulkanRenderDeviceUploadManager>& GetUploadManager() const { return m_UploadManager; }

		VkQueue GetPresentQueue() const { return m_PresentQueue; }

		VulkanAllocator& GetAllocator() { return m_Allocator; }

		uint32_t GetMinUniformBufferOffsetAlignment() const { return m_DeviceInfo.MinUniformBufferAlignment; }
		float GetTimestampPeriod() const { return m_DeviceInfo.TimestampPeriod; }
	private:
		void SubmitWorkToGPUImmediate(VkQueue queueHandle, size_t commandBufferCount, void* commandBufferHandles) const;

		void PickDeviceByRanking(const std::vector<VkPhysicalDevice>& devices);
		void CreateLogicalDevice(const std::vector<const char*>& enabledValidationLayers);

		void FindQueueFamilies(VkPhysicalDevice device);
		VulkanDeviceInfo QueryDeviceInfo(VkPhysicalDevice device) const;

		bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
		bool CheckDeviceFormatSupport(VkPhysicalDevice device) const;
		void PrintDeviceInfo();

		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkDevice m_LogicalDevice = VK_NULL_HANDLE;

		VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
		VkQueue m_PresentQueue = VK_NULL_HANDLE;
		VkQueue m_ComputeQueue = VK_NULL_HANDLE;
		VkQueue m_TransferQueue = VK_NULL_HANDLE;

		std::vector<const char*> m_DeviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME, 
			VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
			VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME,
			VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
			VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
			VK_KHR_COMPUTE_SHADER_DERIVATIVES_EXTENSION_NAME,
			VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
			VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
			VK_KHR_RAY_QUERY_EXTENSION_NAME,
			VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME
		};

		std::vector<VkFormat> m_DeviceFormatSupportToCheck = {
			VK_FORMAT_R8G8B8A8_SRGB,
			VK_FORMAT_R8G8B8A8_SNORM,
			VK_FORMAT_B8G8R8A8_SRGB,
			VK_FORMAT_B8G8R8A8_SNORM,
		};

		VulkanAllocator m_Allocator;
		VulkanDeviceInfo m_DeviceInfo;
		QueueFamilyIndices m_QueueFamilyIndices;
			
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

		VkFence m_ImmediateSubmitFence = VK_NULL_HANDLE;

		Ref<VulkanTransientCommandPool> m_TransientCommandPool = nullptr;

		Unique<VulkanDescriptorSetManager> m_DescriptorSetManager = nullptr;
		Unique<VulkanRenderDeviceUploadManager> m_UploadManager = nullptr;
	};
}