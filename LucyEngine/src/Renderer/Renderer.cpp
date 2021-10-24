#include "Renderer.h"

#include "Context/RendererAPI.h"
#include "Buffer/FrameBuffer.h"
#include "Buffer/RenderBuffer.h"

#include "Buffer/OpenGL/OpenGLFrameBuffer.h"

#include "Texture/Texture.h"
#include "../Utils.h"

#include "RenderPass.h"
#include "Shader/Shader.h"

#include <iostream>
#include "glad/glad.h"

namespace Lucy {

	RefLucy<RendererAPI> Renderer::m_RendererAPI;
	RefLucy<RenderContext> Renderer::m_RenderContext;
	RefLucy<FrameBuffer> Renderer::m_MainFrameBuffer;

	std::vector<Func> Renderer::m_RenderQueue;
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
			renderBufferSpecs.Attachment = GL_DEPTH_STENCIL_ATTACHMENT;
			renderBufferSpecs.InternalFormat = GL_DEPTH24_STENCIL8;
			renderBufferSpecs.Width = width;
			renderBufferSpecs.Height = height;
			renderBufferSpecs.Samples = 4;

			TextureSpecification textureAntialiased;
			textureAntialiased.Width = width;
			textureAntialiased.Height = height;
			textureAntialiased.AttachmentIndex = 0;
			textureAntialiased.Format = { GL_RGBA8, GL_RGBA };
			textureAntialiased.PixelType = PixelType::UnsignedByte;

			TextureSpecification finalTextureSpec;
			finalTextureSpec.Width = width;
			finalTextureSpec.Height = height;
			finalTextureSpec.GenerateMipmap = true;
			finalTextureSpec.AttachmentIndex = 0;
			finalTextureSpec.Parameter.Min = GL_LINEAR;
			finalTextureSpec.Parameter.Mag = GL_LINEAR;
			finalTextureSpec.Format = { GL_RGBA8, GL_RGBA };
			finalTextureSpec.PixelType = PixelType::UnsignedByte;

			FrameBufferSpecification frameBufferSpecs;
			frameBufferSpecs.MultiSampled = true;
			frameBufferSpecs.RenderBuffer = RenderBuffer::Create(renderBufferSpecs);
			frameBufferSpecs.TextureSpecs.push_back(textureAntialiased);
			frameBufferSpecs.BlittedTextureSpecs = finalTextureSpec;

			m_MainFrameBuffer = FrameBuffer::Create(frameBufferSpecs);
		}

		//------------ Shaders ------------
		{
			RefLucy<Shader> testShader = Shader::Create("assets/shaders/LucyBasicShader.glsl");
		}

		//------------ Geometry ------------
		{
			PipelineSpecification geometryPipelineSpecs;
			
			std::vector<ShaderLayoutElement> vertexLayout = {
					{ "a_Vertex", 3 },
					{ "a_Color", 4 }
			};
			
			geometryPipelineSpecs.VertexShaderLayout = VertexShaderLayout(vertexLayout);
			geometryPipelineSpecs.Topology = Topology::LINES;
			geometryPipelineSpecs.Rasterization = { false, 1.0f, GL_FILL };

			RefLucy<Pipeline> geometryPipeline = Pipeline::Create(geometryPipelineSpecs);

			RenderPassSpecification geometryPassSpecs;
			geometryPassSpecs.FrameBuffer = m_MainFrameBuffer;
			geometryPassSpecs.Pipeline = geometryPipeline;
			geometryPassSpecs.ClearColor = { 1.0f, 0.0f, 0.0f, 1.0f };

			RefLucy<RenderPass> geometryPass = RenderPass::Create(geometryPassSpecs);
		}

		Renderer::Dispatch(); //just for init functions
	}

	void Renderer::Submit(const Func&& func)
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
		for (Func func : m_RenderQueue) {
			func();
		}
		m_RenderQueue.clear();
	}

	void Renderer::Destroy()
	{
		glfwTerminate();
	}
}