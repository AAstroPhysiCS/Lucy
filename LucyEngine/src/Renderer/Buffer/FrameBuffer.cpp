#include "lypch.h"
#include "FrameBuffer.h"

#include "OpenGL/OpenGLFrameBuffer.h"
#include "Vulkan/VulkanFrameBuffer.h"
#include "../Renderer.h"

namespace Lucy {

	RefLucy<FrameBuffer> FrameBuffer::Create(FrameBufferSpecification& specs, RefLucy<RenderPass> renderPass) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return CreateRef<OpenGLFrameBuffer>(specs);
				break;
			case RenderArchitecture::Vulkan:
				LUCY_ASSERT(renderPass);
				auto& vulkanRenderPass = As(renderPass, VulkanRenderPass);
				return CreateRef<VulkanFrameBuffer>(specs, vulkanRenderPass);
				break;
		}
		return nullptr;
	}

	FrameBuffer::FrameBuffer(FrameBufferSpecification& specs)
		: m_Specs(specs) {
	}
}