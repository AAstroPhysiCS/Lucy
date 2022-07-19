#include "lypch.h"
#include "IndexBuffer.h"

#include "OpenGL/OpenGLIndexBuffer.h"
#include "Vulkan/VulkanIndexBuffer.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	Ref<IndexBuffer> IndexBuffer::Create(uint32_t size) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return Memory::CreateRef<OpenGLIndexBuffer>(size);
				break;
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanIndexBuffer>(size);
				break;
		}
		return nullptr;
	}

	IndexBuffer::IndexBuffer(uint32_t size) {
		Resize(size); //internal std::vector allocation
	}
}