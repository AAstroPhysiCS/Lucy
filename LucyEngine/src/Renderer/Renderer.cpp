#include "Renderer.h"

#include "Context/RendererAPI.h"
#include "Buffer/FrameBuffer.h"
#include "Buffer/RenderBuffer.h"

#include "Buffer/OpenGL/OpenGLFrameBuffer.h"

#include "Texture/Texture.h"
#include "../Utils.h"

#include "RenderPass.h"
#include "Shader/Shader.h"
#include "Mesh.h"

#include <iostream>
#include "glad/glad.h"

namespace Lucy {

	RefLucy<RendererAPI> Renderer::s_RendererAPI;
	RefLucy<RenderContext> Renderer::s_RenderContext;
	RefLucy<FrameBuffer> Renderer::s_MainFrameBuffer;

	std::vector<Func> Renderer::s_RenderQueue;

	Scene Renderer::m_Scene;
	ShaderLibrary Renderer::m_ShaderLibrary;

	void Renderer::Init(RenderContextType renderContext)
	{
		s_RenderContext = RenderContext::Create(renderContext);
		s_RenderContext->PrintInfo();
		s_RendererAPI = RendererAPI::Create();

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

			s_MainFrameBuffer = FrameBuffer::Create(frameBufferSpecs);
		}

		//------------ Shaders ------------
		{
			Shader::Create("LucyBasicShader", "assets/shaders/LucyBasicShader.glsl");
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

			RefLucy<Pipeline>& geometryPipeline = Pipeline::Create(geometryPipelineSpecs);

			RenderPassSpecification geometryPassSpecs;
			geometryPassSpecs.FrameBuffer = s_MainFrameBuffer;
			geometryPassSpecs.Pipeline = geometryPipeline;
			geometryPassSpecs.ClearColor = { 1.0f, 0.0f, 0.0f, 1.0f };

			RefLucy<RenderPass>& geometryPass = RenderPass::Create(geometryPassSpecs);
		}

		RefLucy<Mesh> mesh = Mesh::Create("assets/models/Sponza/Sponza.gltf");

		Renderer::Dispatch(); //just for init functions
	}

	void Renderer::Submit(const Func&& func)
	{
		s_RenderQueue.push_back(func);
	}

	void Renderer::SubmitMesh()
	{
		//later
		Submit([]() {
		});
	}

	void Renderer::Dispatch() {
		for (Func func : s_RenderQueue) {
			func();
		}
		s_RenderQueue.clear();
	}

	void Renderer::Destroy()
	{
		glfwTerminate();
	}
}