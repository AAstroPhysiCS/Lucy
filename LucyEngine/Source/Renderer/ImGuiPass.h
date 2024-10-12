#pragma once

#include "Renderer/Descriptors/VulkanDescriptorPool.h"

namespace Lucy {

	class VulkanSwapChain;

	constexpr auto IMGUI_MAX_POOL_SIZES = 100u;

	struct ImGuiVulkanImpl final {
		void Init(const Ref<RenderContext>& renderContext);
		void Render(const VulkanSwapChain& swapChain, const RenderCommandList& cmdList);
		void Destroy();

		VulkanDescriptorPoolCreateInfo PoolSpecs = { {}, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, IMGUI_MAX_POOL_SIZES };
		Ref<VulkanDescriptorPool> ImGuiPool = nullptr;
	};
}

