#pragma once

#include "Renderer/Descriptors/VulkanDescriptorPool.h"

namespace Lucy {

	class VulkanSwapChain;

	struct ImGuiVulkanImpl final {
		void Init(RendererBackend* backend);
		void Render(const Ref<VulkanSwapChain>& swapChain, RenderCommandList& cmdList);
		void Destroy();

		Ref<VulkanDescriptorPool> ImGuiPool = nullptr;
	};
}

