#include "lypch.h"
#include "SwapChain.h"
#include "VulkanSwapChain.h"

namespace Lucy {
	
	Ref<SwapChain> SwapChain::Create(RenderArchitecture arch, const Ref<Window>& window, const Ref<RenderDevice>& renderDevice) {
		switch (arch) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanSwapChain>(window, renderDevice);
				break;
			default:
				LUCY_ASSERT(false, "No suitable API found to create the render context!");
				break;
		}
		return nullptr;
	}

	SwapChain::SwapChain(const Ref<Window>& window, const Ref<RenderDevice>& renderDevice)
		: m_Window(window), m_RenderDevice(renderDevice) {
	}
}
