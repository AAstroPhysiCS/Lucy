#pragma once

#include "RendererBase.h"
#include "Synchronization/VulkanSyncItems.h"

#include "Memory/VulkanAllocator.h"

namespace Lucy {

	class VulkanRenderer : public RendererBase {
	public:
		VulkanRenderer(RenderArchitecture arch, Ref<Window>& window);
		virtual ~VulkanRenderer() = default;

		void BeginScene(Ref<Scene>& scene) final override;
		void RenderScene() final override;
		RenderContextResultCodes EndScene() final override;
		void WaitForDevice() final override;

		void Destroy() final override;

		void DirectCopyBuffer(VkBuffer& stagingBuffer, VkBuffer& buffer, VkDeviceSize size);
		void SubmitImmediateCommand(std::function<void(VkCommandBuffer)>&& func);

		void OnWindowResize() final override;
		void OnViewportResize() final override;
		Entity OnMousePicking(Ref<Scene>& scene, const Ref<GraphicsPipeline>& idPipeline) final override;
	private:
		std::vector<Semaphore> m_WaitSemaphores;
		std::vector<Semaphore> m_SignalSemaphores;
		std::vector<Fence> m_InFlightFences;

		VkResult m_LastSwapChainResult = VK_NOT_READY;

		inline static VkBuffer m_IDBuffer = VK_NULL_HANDLE;
		inline static VmaAllocation m_IDBufferVma = VK_NULL_HANDLE;
	};
}