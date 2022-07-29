#include "lypch.h"
#include "VertexBuffer.h"
#include "Vulkan/VulkanVertexBuffer.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	Ref<VertexBuffer> VertexBuffer::Create(uint32_t size) {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanVertexBuffer>(size);
				break;
		}
		return nullptr;
	}
	
	VertexBuffer::VertexBuffer(uint32_t size) {
		Resize(size); //internal std::vector allocation
	}
}