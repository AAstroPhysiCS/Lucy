#include "lypch.h"
#include "RenderBuffer.h"

#include "Renderer/Renderer.h"
#include "OpenGL/OpenGLRenderBuffer.h"

namespace Lucy {

	RenderBuffer::RenderBuffer(const RenderBufferCreateInfo& createInfo) :
		m_CreateInfo(createInfo) {
	}

	Ref<RenderBuffer> RenderBuffer::Create(const RenderBufferCreateInfo& createInfo) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return Memory::CreateRef<OpenGLRenderBuffer>(createInfo);
				break;
		}
		return nullptr;
	}
}