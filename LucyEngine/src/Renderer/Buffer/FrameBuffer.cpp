#include "lypch.h"
#include "FrameBuffer.h"

#include "OpenGL/OpenGLFrameBuffer.h"
#include "Vulkan/VulkanFrameBuffer.h"
#include "../Renderer.h"

namespace Lucy {

	RefLucy<FrameBuffer> FrameBuffer::Create(FrameBufferSpecification& specs) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return CreateRef<OpenGLFrameBuffer>(specs);
				break;
			case RenderArchitecture::Vulkan:
				return CreateRef<VulkanFrameBuffer>(specs);
				break;
			default:
				LUCY_ASSERT(false);
				break;
		}
	}

	FrameBuffer::FrameBuffer(FrameBufferSpecification& specs)
		: m_Specs(specs) {
	}
}