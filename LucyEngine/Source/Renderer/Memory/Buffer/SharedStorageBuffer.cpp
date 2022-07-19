#include "lypch.h"
#include "SharedStorageBuffer.h"

#include "Renderer/Renderer.h"
#include "Vulkan/VulkanSharedStorageBuffer.h"

namespace Lucy {

	Ref<SharedStorageBuffer> SharedStorageBuffer::Create(const SharedStorageBufferCreateInfo& createInfo) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				LUCY_ASSERT(false);
				//return Memory::CreateRef<OpenGLSharedStorageBuffer>(createInfo);
				break;
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