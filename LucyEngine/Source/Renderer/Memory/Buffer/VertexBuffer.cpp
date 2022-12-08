#include "lypch.h"
#include "VertexBuffer.h"
#include "Vulkan/VulkanVertexBuffer.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	Ref<VertexBuffer> VertexBuffer::Create(size_t size) {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanVertexBuffer>(size);
				break;
		}
		return nullptr;
	}
	
	VertexBuffer::VertexBuffer(size_t size) {
		Resize(size); //internal std::vector allocation
	}
}