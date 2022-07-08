#include "lypch.h"
#include "VertexBuffer.h"

#include "OpenGL/OpenGLVertexBuffer.h"
#include "Vulkan/VulkanVertexBuffer.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	Ref<VertexBuffer> VertexBuffer::Create(uint32_t size) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return Memory::CreateRef<OpenGLVertexBuffer>(size);
				break;
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanVertexBuffer>(size);
				break;
		}
		return nullptr;
	}
	
	VertexBuffer::VertexBuffer(uint32_t size) {
		Allocate(size); //internal std::vector allocation
	}
}