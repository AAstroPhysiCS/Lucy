#include "lypch.h"
#include "FrameBuffer.h"

#include "OpenGL/OpenGLFrameBuffer.h"
#include "Vulkan/VulkanFrameBuffer.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	FrameBuffer::FrameBuffer(FrameBufferSpecification& specs)
		: m_Specs(specs) {
	}

	Ref<FrameBuffer> FrameBuffer::Create(FrameBufferSpecification& specs) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return Memory::CreateRef<OpenGLFrameBuffer>(specs);
				break;
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanFrameBuffer>(specs);
				break;
			default:
				LUCY_CRITICAL("Other API's are not supported!");
				LUCY_ASSERT(false);
				break;
		}
		return nullptr;
	}
}