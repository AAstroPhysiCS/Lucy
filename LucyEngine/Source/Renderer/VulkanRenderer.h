#pragma once

#include "RendererBase.h"

namespace Lucy {

	class VulkanRenderer : public RendererBase {
	public:
		VulkanRenderer(RenderArchitecture arch, Ref<Window>& window);
		virtual ~VulkanRenderer() = default;

		void BeginScene(Ref<Scene>& scene) final override;
		void RenderScene() final override;
		PresentResult EndScene() final override;
		void WaitForDevice() final override;

		void DirectCopyBuffer(VkBuffer& stagingBuffer, VkBuffer& buffer, VkDeviceSize size);
		void ExecuteSingleTimeCommand(std::function<void(VkCommandBuffer)>&& func);

		void OnWindowResize() final override;
		void OnViewportResize() final override;
		Entity OnMousePicking(Ref<Scene>& scene, const Ref<Pipeline>& idPipeline) final override;
	};
}