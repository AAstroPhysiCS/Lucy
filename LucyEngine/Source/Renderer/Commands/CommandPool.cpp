#include "lypch.h"
#include "CommandPool.h"

#include "VulkanCommandPool.h"
#include "Renderer/Renderer.h"

namespace Lucy {

	Ref<CommandPool> CommandPool::Create(const CommandPoolCreateInfo& createInfo) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanCommandPool>(createInfo);
		}
		LUCY_ASSERT(false);
		return nullptr;
	}

	CommandPool::CommandPool(const CommandPoolCreateInfo& createInfo)
		: m_CreateInfo(createInfo) {
	}
}