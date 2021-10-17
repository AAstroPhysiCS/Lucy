#include "Renderer.h"

#include "Context/RendererAPI.h"
#include "Buffer/FrameBuffer.h"
#include "Buffer/RenderBuffer.h"

#include "Buffer/OpenGL/OpenGLFrameBuffer.h"

#include "Texture/Texture.h"
#include "../Utils.h"

#include "RenderPass.h"

#include <iostream>

namespace Lucy {

	RefLucy<RendererAPI> Renderer::m_RendererAPI;
	RefLucy<RenderContext> Renderer::m_RenderContext;
	RefLucy<FrameBuffer> Renderer::m_MainFrameBuffer;

	std::vector<RenderFunc> Renderer::m_RenderQueue;
	Scene Renderer::m_Scene;

	void Renderer::Init(RenderContextType renderContext)
	{
		m_RenderContext = RenderContext::Create(renderContext);
		m_RenderContext->PrintInfo();
		m_RendererAPI = RendererAPI::Create();

		auto [width, height] = Utils::ReadSizeFromIni("Viewport");

		//------------ Main Framebuffer ------------
		{
			RenderBufferSpecification renderBufferSpecs;
			renderBufferSpecs.attachment = GL_DEPTH_STENCIL_ATTACHMENT;
			renderBufferSpecs.internalFormat = GL_DEPTH24_STENCIL8;
			renderBufferSpecs.width = width;
			renderBufferSpecs.height = height;
			renderBufferSpecs.samples = 4;

			TextureSpecification textureAntialiased;
			textureAntialiased.width = width;
			textureAntialiased.height = height;
			textureAntialiased.attachmentIndex = 0;
			textureAntialiased.format = { GL_RGBA8, GL_RGBA };
			textureAntialiased.pixelType = PixelType::UnsignedByte;

			TextureSpecification finalTextureSpec;
			finalTextureSpec.width = width;
			finalTextureSpec.height = height;
			finalTextureSpec.generateMipmap = true;
			finalTextureSpec.attachmentIndex = 0;
			finalTextureSpec.parameter.min = GL_LINEAR;
			finalTextureSpec.parameter.mag = GL_LINEAR;
			finalTextureSpec.format = { GL_RGBA8, GL_RGBA };
			finalTextureSpec.pixelType = PixelType::UnsignedByte;

			FrameBufferSpecification frameBufferSpecs;
			frameBufferSpecs.multiSampled = true;
			frameBufferSpecs.renderBuffer = RenderBuffer::Create(renderBufferSpecs);
			frameBufferSpecs.textureSpecs.push_back(textureAntialiased);
			frameBufferSpecs.blittedTextureSpecs = finalTextureSpec;

			m_MainFrameBuffer = FrameBuffer::Create(frameBufferSpecs);
		}

		//------------ Geometry ------------
		{

			PipelineSpecification geometryPipelineSpecs;
			Pipeline geometryPipeline(geometryPipelineSpecs);

			RenderPassSpecification geometryPassSpecs;
			geometryPassSpecs.frameBuffer = m_MainFrameBuffer;
			geometryPassSpecs.pipeline = &geometryPipeline;
			geometryPassSpecs.clearColor = { 1.0f, 0.0f, 0.0f, 1.0f };

			RenderPass geometryPass = RenderPass::Create(geometryPassSpecs);
		}

		Renderer::Dispatch(); //just for init
	}

	void Renderer::Submit(const RenderFunc&& func)
	{
		m_RenderQueue.push_back(func);
	}

	void Renderer::SubmitMesh()
	{
		//later
		Submit([]() {
		});
	}

	void Renderer::Dispatch() {
		for (RenderFunc func : m_RenderQueue) {
			func();
		}
		m_RenderQueue.clear();
	}

	void Renderer::Destroy()
	{
		glfwTerminate();
	}
}