#include "lypch.h"
#include "IndexBuffer.h"

#include "Vulkan/VulkanIndexBuffer.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	Ref<IndexBuffer> IndexBuffer::Create(size_t size) {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanIndexBuffer>(size);
				break;
		}
		return nullptr;
	}

	IndexBuffer::IndexBuffer(size_t size) {
		Resize(size); //internal std::vector allocation
	}
}