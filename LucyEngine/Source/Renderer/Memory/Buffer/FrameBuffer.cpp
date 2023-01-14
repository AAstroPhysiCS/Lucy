#include "lypch.h"
#include "FrameBuffer.h"
#include "Vulkan/VulkanFrameBuffer.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	Ref<FrameBuffer> FrameBuffer::Create(FrameBufferCreateInfo& createInfo) {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanFrameBuffer>(createInfo);
				break;
			default:
				LUCY_ASSERT(false, "No suitable API found to create the resource!");
				break;
		}
		return nullptr;
	}

	FrameBuffer::FrameBuffer(FrameBufferCreateInfo& createInfo)
		: m_CreateInfo(createInfo) {
	}
}