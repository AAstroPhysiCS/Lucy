#pragma once

namespace Lucy {

	class RendererBackend;
	class RenderCommandList;

	class VulkanDescriptorPool;
	class VulkanSwapChain;

	struct ImGuiVulkanImpl final {
		void Init(RendererBackend* backend);
		void Render(const Ref<VulkanSwapChain>& swapChain, RenderCommandList& cmdList);
		void Destroy();

		Ref<VulkanDescriptorPool> ImGuiPool = nullptr;
	};
}