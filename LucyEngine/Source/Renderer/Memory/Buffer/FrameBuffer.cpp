#include "lypch.h"
#include "FrameBuffer.h"

#include "Core/Base.h"

#include "OpenGL/OpenGLFrameBuffer.h"
#include "Vulkan/VulkanFrameBuffer.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	FrameBuffer::FrameBuffer(FrameBufferCreateInfo& createInfo)
		: m_CreateInfo(createInfo) {
	}

	Ref<FrameBuffer> FrameBuffer::Create(FrameBufferCreateInfo& createInfo) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return Memory::CreateRef<OpenGLFrameBuffer>(createInfo);
				break;
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanFrameBuffer>(createInfo);
				break;
			default:
				LUCY_CRITICAL("Other API's are not supported!");
				LUCY_ASSERT(false);
				break;
		}
		return nullptr;
	}
}