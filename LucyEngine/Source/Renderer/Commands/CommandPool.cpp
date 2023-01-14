#include "lypch.h"
#include "CommandPool.h"

#include "VulkanCommandPool.h"
#include "Renderer/Renderer.h"

namespace Lucy {

	Ref<CommandPool> CommandPool::Create(const CommandPoolCreateInfo& createInfo) {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanCommandPool>(createInfo);
			default:
				LUCY_ASSERT(false, "No suitable API found to create the resource!");
		}
		return nullptr;
	}

	CommandPool::CommandPool(const CommandPoolCreateInfo& createInfo)
		: m_CreateInfo(createInfo) {
	}
}