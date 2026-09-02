#pragma once

#include "RendererBackend.h"
#include "Context/VulkanSwapChain.h"

#include "Semaphore.h"

#include "Memory/VulkanAllocator.h"

#include "ImGuiPass.h"

namespace Lucy {

	class VulkanTransientCommandPool;

	class VulkanRenderer : public RendererBackend {
	public:
		VulkanRenderer(RendererConfiguration config, const Ref<Window>& window);
		virtual ~VulkanRenderer() = default;

		VulkanRenderer(const VulkanRenderer& other) = delete;
		VulkanRenderer(VulkanRenderer&& other) noexcept = delete;
		VulkanRenderer& operator=(const VulkanRenderer& other) = delete;
		VulkanRenderer& operator=(VulkanRenderer&& other) noexcept = delete;

		void SubmitBatchesToRender(std::vector<ExecutionBatch>& batches, const std::unordered_map<std::string, RenderFrameHandles>& renderFrameHandleMap) final override;
		RenderContextResultCodes WaitAndPresent() final override;

		void Destroy() final override;

		void SubmitImmediateCommand(std::function<void(VkCommandBuffer)>&& func);

		void OnWindowResize() final override;
		void OnViewportResize() final override;
		uint32_t OnMousePicking(const EntityPickedEvent& e, const Ref<Image>& currentFrameBufferImage) final override;

		void InitializeImGui() final override;
	private:
		void Init() final override;

		void BeginFrame() final override;
		void RenderFrame() final override;
		void EndFrame() final override;

		void FlushDeletionQueue() final override;

		void ProcessQueryResults();

		void InternalImGuiPass(uint64_t signalValue, bool hasSceneWork);

		void LinkBatches(RenderSubmitQueue& submitQueue);
		void ExecuteVulkanBatchBarrier(VkCommandBuffer cmdBuffer, const VulkanBatchBarrier& barrier);

		/*
		* imageAvailable[frame] -> from acquire, waited by first GPU submit of the frame
		* sceneFinished[frame] -> signaled by last scene batch, waited by ImGui
		* renderFinished[image] -> signaled by ImGui, waited by present
		* frameFence[frame] timeline -> signaled by ImGui, waited next reuse of that frame slot
		*/

		std::vector<VulkanSemaphore> m_ImageAvailableSemaphores;
		std::vector<VulkanSemaphore> m_RenderFinishedSemaphores;
		std::vector<VulkanSemaphore> m_InFlightFences;

		std::array<VulkanSemaphore, static_cast<size_t>(TargetQueueFamily::Count)> m_QueueSemaphores;
		std::array<uint64_t, static_cast<size_t>(TargetQueueFamily::Count)> m_QueueSemaphoreValues{};

		std::vector<uint64_t> m_FrameFenceValues;

		RenderContextResultCodes m_LastSwapChainResult = RenderContextResultCodes::SUCCESS;

		ImGuiVulkanImpl m_ImGuiPassImpl;
		Unique<RenderCommandList> m_ImGuiRenderCommandList;

		static inline VkBuffer s_IDBuffer = VK_NULL_HANDLE;
		static inline VmaAllocation s_IDBufferVma = VK_NULL_HANDLE;
	};
}