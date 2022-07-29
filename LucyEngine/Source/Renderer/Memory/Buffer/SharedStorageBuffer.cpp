#include "lypch.h"
#include "SharedStorageBuffer.h"
#include "Vulkan/VulkanSharedStorageBuffer.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	Ref<SharedStorageBuffer> SharedStorageBuffer::Create(const SharedStorageBufferCreateInfo& createInfo) {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanSharedStorageBuffer>(createInfo);
				break;
		}
		return nullptr;
	}

	SharedStorageBuffer::SharedStorageBuffer(const SharedStorageBufferCreateInfo& createInfo)
		: m_CreateInfo(createInfo) {
		Reserve(m_CreateInfo.BufferSize);
	}
}