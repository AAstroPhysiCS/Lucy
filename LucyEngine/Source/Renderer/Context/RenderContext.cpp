#include "lypch.h"

#include "RenderContext.h"
#include "VulkanContext.h"

namespace Lucy {
	
	Ref<RenderContext> RenderContext::Create(RenderArchitecture arch, const Ref<Window>& window) {
		switch (arch) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanContext>(window);
				break;
			default:
				LUCY_ASSERT(false, "No suitable API found to create the render context!");
				break;
		}
		return nullptr;
	}

	RenderContext::RenderContext(const Ref<Window>& window, Ref<RenderDevice> renderDevice)
		: m_Window(window), m_RenderDevice(renderDevice) {
	}

	RenderContext::~RenderContext() {
		m_RenderDevice->Destroy();
	}
}