#include "lypch.h"

#include "RenderContext.h"
#include "VulkanContext.h"

namespace Lucy {

	Ref<RenderContext> RenderContext::Create(RenderArchitecture arch, Ref<Window>& window) {
		switch (arch) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanContext>(window);
				break;
			default:
				LUCY_ASSERT(false, "No suitable API found to create the resource!");
				break;
		}
		return nullptr;
	}

	RenderContext::RenderContext(Ref<Window>& window)
		: m_Window(window) {
	}
}