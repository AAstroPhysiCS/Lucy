#pragma once

#include "RenderDevice.h"
#include "Renderer/Memory/VulkanAllocator.h"

#include "Renderer/Memory/Buffer/PushConstant.h"

#include "Renderer/Synchronization/VulkanSyncItems.h"

namespace Lucy {

	class Mesh;

	class VulkanTransientCommandPool;

	struct VulkanDeviceInfo {
		std::string Name;
		uint32_t DriverVersion = 0;
		uint32_t ApiVersion = 0;
		uint32_t MinUniformBufferAlignment = 0;
		float TimestampPeriod = 0;
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

		void Init(VkInstance instance, std::vector<const char*>& enabledValidationLayers, VkSurfaceKHR surface, uint32_t apiVersion);
		void Destroy() final override;

		void BeginCommandBuffer(Ref<CommandPool> cmdPool);
		void EndCommandBuffer(Ref<CommandPool> cmdPool);

		void BindBuffers(Ref<CommandPool> cmdPool, Ref<Mesh> mesh) final override;
		void BindBuffers(Ref<CommandPool> cmdPool, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer) final override;

		void BindPushConstant(Ref<CommandPool> cmdPool, Ref<GraphicsPipeline> pipeline, const VulkanPushConstant& pushConstant) final override;
		void BindPushConstant(Ref<CommandPool> cmdPool, Ref<ComputePipeline> pipeline, const VulkanPushConstant& pushConstant) final override;

		void BindPipeline(Ref<CommandPool> cmdPool, Ref<GraphicsPipeline> pipeline) final override;
		void BindPipeline(Ref<CommandPool> cmdPool, Ref<ComputePipeline> pipeline) final override;

		void UpdateDescriptorSets(Ref<GraphicsPipeline> pipeline) final override;
		void UpdateDescriptorSets(Ref<ComputePipeline> pipeline) final override;

		void BindAllDescriptorSets(Ref<CommandPool> cmdPool, Ref<GraphicsPipeline> pipeline) final override;
		void BindDescriptorSet(Ref<CommandPool> cmdPool, Ref<GraphicsPipeline> pipeline, uint32_t setIndex) final override;
		
		void BindAllDescriptorSets(Ref<CommandPool> cmdPool, Ref<ComputePipeline> pipeline) final override;
		void BindDescriptorSet(Ref<CommandPool> cmdPool, Ref<ComputePipeline> pipeline, uint32_t setIndex) final override;

		void DrawIndexed(Ref<CommandPool> cmdPool, uint32_t indexCount, uint32_t instanceCount,
						 uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) final override;
		void DispatchCompute(Ref<CommandPool> cmdPool, Ref<ComputePipeline> computePipeline, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) final override;
		
		void BeginRenderPass(Ref<RenderPass> renderPass, Ref<FrameBuffer> frameBuffer, Ref<CommandPool> cmdPool) final override;
		void EndRenderPass(Ref<RenderPass> renderPass) final override;

		void BeginDebugMarker(Ref<CommandPool> cmdPool, const char* labelName) final override;
		void EndDebugMarker(Ref<CommandPool> cmdPool) final override;

		void SubmitWorkToGPU(TargetQueueFamily queueFamily, Ref<CommandPool> cmdPool,
							 const Fence& currentFrameFence, const Semaphore& currentFrameWaitSemaphore, const Semaphore& currentFrameSignalSemaphore) final override;
		void SubmitWorkToGPU(TargetQueueFamily queueFamily, std::vector<Ref<CommandPool>>& cmdPools,
							 const Fence& currentFrameFence, const Semaphore& currentFrameWaitSemaphore, const Semaphore& currentFrameSignalSemaphore) final override;
		void SubmitImmediateCommand(const std::function<void(VkCommandBuffer)>& func, const Ref<VulkanTransientCommandPool>& cmdPool);
		void WaitForDevice() final override;
		void WaitForQueue(TargetQueueFamily queueFamily) final override;

		inline VulkanDeviceInfo& GetDeviceInformation() { return m_DeviceInfo; }

		inline VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
		inline VkDevice GetLogicalDevice() const { return m_LogicalDevice; }
		inline QueueFamilyIndices GetQueueFamilies() const { return m_QueueFamilyIndices; }

		inline VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
		inline VkQueue GetPresentQueue() const { return m_PresentQueue; }
		inline VkQueue GetComputeQueue() const { return m_ComputeQueue; }
		inline VkQueue GetTransferQueue() const { return m_TransferQueue; }

		inline VulkanAllocator& GetAllocator() { return m_Allocator; }

		inline uint32_t GetMinUniformBufferOffsetAlignment() const { return m_DeviceInfo.MinUniformBufferAlignment; }
		inline float GetTimestampPeriod() const { return m_DeviceInfo.TimestampPeriod; }
	private:
		void SubmitWorkToGPU(VkQueue queueHandle, size_t commandBufferCount, VkCommandBuffer* commandBuffers, const Fence& currentFrameFence, const Semaphore& currentFrameWaitSemaphore, const Semaphore& currentFrameSignalSemaphore) const;
		void SubmitWorkToGPU(VkQueue queueHandle, VkCommandBuffer currentCommandBuffer, const Fence& currentFrameFence, const Semaphore& currentFrameWaitSemaphore, const Semaphore& currentFrameSignalSemaphore) const;
		void SubmitWorkToGPU(VkQueue queueHandle, size_t commandBufferCount, void* commandBufferHandles) const;

		void PickDeviceByRanking(const std::vector<VkPhysicalDevice>& devices);
		void CreateLogicalDevice(std::vector<const char*>& enabledValidationLayers);

		void FindQueueFamilies(VkPhysicalDevice device);

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
			VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME
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

		Unique<Fence> m_ImmediateCommandFence = nullptr;
	};
}