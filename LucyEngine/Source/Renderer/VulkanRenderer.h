#pragma once

#include "RendererBackend.h"
#include "Context/VulkanSwapChain.h"
#include "Synchronization/VulkanSyncItems.h"

#include "Memory/VulkanAllocator.h"

#include "ImGuiPass.h"

namespace Lucy {

	class VulkanTransientCommandPool;

	class VulkanRenderer : public RendererBackend {
	public:
		VulkanRenderer(RendererConfiguration config, const Ref<Window>& window);
		virtual ~VulkanRenderer() = default;

		RenderContextResultCodes WaitAndPresent() final override;

		void ExecuteBarrier(void* commandBufferHandle, Ref<Image> image) final override;
		void ExecuteBarrier(void* commandBufferHandle, void* imageHandle, uint32_t imageLayout, uint32_t layerCount, uint32_t mipCount) final override;

		void Destroy() final override;

		void RTDirectCopyBuffer(VkBuffer& stagingBuffer, VkBuffer& buffer, VkDeviceSize size);
		void SubmitImmediateCommand(std::function<void(VkCommandBuffer)>&& func);

		void OnWindowResize() final override;
		void OnViewportResize() final override;
		glm::vec3 OnMousePicking(const EntityPickedEvent& e, const Ref<Image>& currentFrameBufferImage) final override;

		void InitializeImGui() final override;
		void RTRenderImGui() final override;
	private:
		void Init() final override;

		void BeginFrame() final override;
		void RenderFrame() final override;
		void EndFrame() final override;

		void FlushDeletionQueue() final override;

		std::vector<Semaphore> m_WaitSemaphores;
		std::vector<Semaphore> m_SignalSemaphores;
		std::vector<Fence> m_InFlightFences;

		std::vector<Semaphore> m_WaitSemaphoresCompute;
		std::vector<Semaphore> m_SignalSemaphoresCompute;
		std::vector<Fence> m_InFlightFencesCompute;

		bool m_UseComputeSemaphore = false;

		RenderContextResultCodes m_LastSwapChainResult = RenderContextResultCodes::SUCCESS;

		Ref<VulkanTransientCommandPool> m_TransientCommandPool = nullptr;

		ImGuiVulkanImpl m_ImGuiPass;

		static inline VkBuffer s_IDBuffer = VK_NULL_HANDLE;
		static inline VmaAllocation s_IDBufferVma = VK_NULL_HANDLE;
	};
}