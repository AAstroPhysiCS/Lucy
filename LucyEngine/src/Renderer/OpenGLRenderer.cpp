#include "lypch.h"
#include "OpenGLRenderer.h"

#include "Buffer/FrameBuffer.h"
#include "Buffer/RenderBuffer.h"
#include "Buffer/UniformBuffer.h"
#include "Mesh.h"

#include "OpenGLRenderPass.h"

#include "glad/glad.h"

#include "Utils.h"

namespace Lucy {

	OpenGLRenderer::OpenGLRenderer(RenderArchitecture renderArchitecture)
		: RendererAPI(renderArchitecture)
	{
	}

	void OpenGLRenderer::Init() {
		m_RenderContext = RenderContext::Create(m_Architecture);
		m_RenderContext->PrintInfo();

		RendererAPI::Init(); //call for uniform buffers

		Shader::Create("LucyPBR", "assets/shaders/LucyPBR.glsl");
		Shader::Create("LucyID", "assets/shaders/LucyID.glsl");

		auto [width, height] = Utils::ReadSizeFromIni("Viewport");

		uint32_t TargetSamples = 4;

		//------------ Main Framebuffer ------------
		{
			RenderBufferSpecification renderBufferSpecs;
			renderBufferSpecs.Attachment = GL_DEPTH_STENCIL_ATTACHMENT;
			renderBufferSpecs.InternalFormat = GL_DEPTH24_STENCIL8;
			renderBufferSpecs.Width = width;
			renderBufferSpecs.Height = height;
			renderBufferSpecs.Samples = TargetSamples;

			TextureSpecification textureAntialiased;
			textureAntialiased.Width = width;
			textureAntialiased.Height = height;
			textureAntialiased.DisableReadWriteBuffer = false;
			textureAntialiased.Samples = TargetSamples;
			textureAntialiased.AttachmentIndex = 0;
			textureAntialiased.Format = { GL_RGBA8, GL_RGBA };
			textureAntialiased.PixelType = PixelType::UnsignedByte;

			TextureSpecification finalTextureSpec;
			finalTextureSpec.Width = width;
			finalTextureSpec.Height = height;
			finalTextureSpec.GenerateMipmap = true;
			finalTextureSpec.DisableReadWriteBuffer = false;
			finalTextureSpec.AttachmentIndex = 0;
			finalTextureSpec.Parameter.Min = GL_LINEAR;
			finalTextureSpec.Parameter.Mag = GL_LINEAR;
			finalTextureSpec.Format = { GL_RGBA8, GL_RGBA };
			finalTextureSpec.PixelType = PixelType::UnsignedByte;

			FrameBufferSpecification geometryFrameBufferSpecs;
			geometryFrameBufferSpecs.MultiSampled = true;
			geometryFrameBufferSpecs.ViewportWidth = width;
			geometryFrameBufferSpecs.ViewportHeight = height;
			geometryFrameBufferSpecs.RenderBuffer = RenderBuffer::Create(renderBufferSpecs);
			geometryFrameBufferSpecs.TextureSpecs.push_back(textureAntialiased);
			geometryFrameBufferSpecs.BlittedTextureSpecs = finalTextureSpec;

			PipelineSpecification geometryPipelineSpecs;
			std::vector<ShaderLayoutElement> vertexLayout = {
					{ "a_Pos", ShaderDataSize::Float3 },
					{ "a_ID", ShaderDataSize::Float3 },
					{ "a_TextureCoords", ShaderDataSize::Float2 },
					{ "a_Normals", ShaderDataSize::Float3 },
					{ "a_Tangents", ShaderDataSize::Float3 },
					{ "a_BiTangents", ShaderDataSize::Float3 }
			};

			geometryPipelineSpecs.VertexShaderLayout = VertexShaderLayout(vertexLayout);
			geometryPipelineSpecs.Topology = Topology::TRIANGLES;
			geometryPipelineSpecs.Rasterization = { true, GL_BACK, 1.0f, PolygonMode::FILL };

			RenderPassSpecification geometryPassSpecs;
			geometryPassSpecs.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
			
			geometryPipelineSpecs.FrameBuffer = FrameBuffer::Create(geometryFrameBufferSpecs);
			geometryPipelineSpecs.RenderPass = RenderPass::Create(geometryPassSpecs);
			m_GeometryPipeline = As(Pipeline::Create(geometryPipelineSpecs), OpenGLPipeline);

			//------------ Mouse Picking ------------
			TextureSpecification idTextureRGBSpecs;
			idTextureRGBSpecs.Width = width;
			idTextureRGBSpecs.Height = height;
			idTextureRGBSpecs.AttachmentIndex = 0;
			idTextureRGBSpecs.Parameter.Min = GL_NEAREST;
			idTextureRGBSpecs.Parameter.Mag = GL_NEAREST;
			idTextureRGBSpecs.DisableReadWriteBuffer = false;
			idTextureRGBSpecs.Format = { GL_RGB32F , GL_RGB };
			idTextureRGBSpecs.PixelType = PixelType::Float;

			TextureSpecification idTextureDepthSpecs;
			idTextureDepthSpecs.Width = width;
			idTextureDepthSpecs.Height = height;
			idTextureDepthSpecs.AttachmentIndex = 1;
			idTextureDepthSpecs.Parameter.Min = GL_NEAREST;
			idTextureDepthSpecs.Parameter.Mag = GL_NEAREST;
			idTextureDepthSpecs.DisableReadWriteBuffer = false;
			idTextureDepthSpecs.Format = { GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT };
			idTextureDepthSpecs.PixelType = PixelType::Float;

			FrameBufferSpecification idFrameBufferSpecs;
			idFrameBufferSpecs.ViewportWidth = width;
			idFrameBufferSpecs.ViewportHeight = height;
			idFrameBufferSpecs.TextureSpecs.push_back(idTextureRGBSpecs);
			idFrameBufferSpecs.TextureSpecs.push_back(idTextureDepthSpecs);
			idFrameBufferSpecs.IsStorage = true;

			RenderPassSpecification idRenderPassSpecs;
			idRenderPassSpecs.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
			
			geometryPipelineSpecs.FrameBuffer = FrameBuffer::Create(idFrameBufferSpecs);
			geometryPipelineSpecs.RenderPass = RenderPass::Create(idRenderPassSpecs);
			m_IDPipeline = As(Pipeline::Create(geometryPipelineSpecs), OpenGLPipeline);
		}

		Dispatch(); //just for init functions (if any)
	}

	void OpenGLRenderer::OnFramebufferResize(float sizeX, float sizeY) {
		m_GeometryPipeline->GetFrameBuffer()->Resize(sizeX, sizeY);
		m_IDPipeline->GetFrameBuffer()->Resize(sizeX, sizeY);
		m_ViewportWidth = sizeX;
		m_ViewportHeight = sizeY;
	}

	Entity OpenGLRenderer::OnMousePicking() {
		m_IDPipeline->GetFrameBuffer()->Bind();
		glm::vec3 pixelValue;
		OpenGLAPICommands::ReadBuffer(GL_COLOR_ATTACHMENT0);
		OpenGLAPICommands::ReadPixels(m_ViewportMouseX, m_ViewportHeight - m_ViewportMouseY, 1, 1, (float*)&pixelValue);

		//checking if the data that is being read make sense
		if ((pixelValue.x > 255.0f || pixelValue.x < 0.0f) ||
			(pixelValue.y > 255.0f || pixelValue.y < 0.0f) ||
			(pixelValue.z > 255.0f || pixelValue.z < 0.0f))
			return {};

		//checking if we clicked on the void
		if (pixelValue.x == 0 && pixelValue.y == 0 && pixelValue.z == 0)
			return {};

		Entity selectedEntity = m_ActiveScene->GetEntityByPixelValue(pixelValue);
		m_IDPipeline->GetFrameBuffer()->Unbind();

		return selectedEntity;
	}

	void OpenGLRenderer::GeometryPass() {
		Pipeline::Begin(m_GeometryPipeline);
		for (MeshDrawCommand meshComponent : m_MeshDrawCommands) {
			const RefLucy<Mesh>& mesh = meshComponent.Mesh;
			std::vector<Material>& materials = mesh->GetMaterials();
			std::vector<Submesh>& submeshes = mesh->GetSubmeshes();

			mesh->Bind();
			for (uint32_t i = 0; i < submeshes.size(); i++) {
				Submesh& submesh = submeshes[i];
				Material& material = materials[submesh.MaterialIndex];
				const RefLucy<Shader>& shader = material.GetShader();

				material.Bind();
				m_CameraUniformBuffer->SetData((void*)&(meshComponent.EntityTransform * submesh.Transform), sizeof(glm::mat4), sizeof(glm::mat4) * 2);
				OpenGLAPICommands::DrawElementsBaseVertex(m_GeometryPipeline->GetTopology(), submesh.IndexCount, submesh.BaseIndexCount, submesh.BaseVertexCount);
				material.Unbind();
			}
			mesh->Unbind();
		}
		Pipeline::End(m_GeometryPipeline);
	}

	void OpenGLRenderer::IDPass() {
		RefLucy<Shader> idShader = m_ShaderLibrary.GetShader("LucyID");
		Pipeline::Begin(m_IDPipeline);

		idShader->Bind();
		for (MeshDrawCommand meshComponent : m_MeshDrawCommands) {
			const RefLucy<Mesh>& mesh = meshComponent.Mesh;
			std::vector<Submesh>& submeshes = mesh->GetSubmeshes();

			mesh->Bind();
			for (uint32_t i = 0; i < submeshes.size(); i++) {
				Submesh& submesh = submeshes[i];
				m_CameraUniformBuffer->SetData((void*)&(meshComponent.EntityTransform * submesh.Transform), sizeof(glm::mat4), sizeof(glm::mat4) * 2);
				OpenGLAPICommands::DrawElementsBaseVertex(m_IDPipeline->GetTopology(), submesh.IndexCount, submesh.BaseIndexCount, submesh.BaseVertexCount);
			}
			mesh->Unbind();
		}
		idShader->Unbind();
		Pipeline::End(m_IDPipeline);
	}

	void OpenGLRenderer::Execute() {
		GeometryPass();
		IDPass();
	}

	void OpenGLRenderer::BeginScene(Scene& scene) {
		EditorCamera& camera = scene.GetEditorCamera();
		camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
		camera.Update();
		m_CameraUniformBuffer->SetData((void*)&camera.GetViewMatrix(), sizeof(glm::mat4), 0);
		m_CameraUniformBuffer->SetData((void*)&camera.GetProjectionMatrix(), sizeof(glm::mat4), sizeof(glm::mat4));

		m_ActiveScene = &scene;
	}

	void OpenGLRenderer::EndScene() {
		Execute();
	}

	void OpenGLRenderer::ClearCommands() {
		m_MeshDrawCommands.clear();
	}

	void OpenGLRenderer::Submit(const Func&& func) {
		m_RenderFunctions.push_back(func);
	}

	void OpenGLRenderer::SubmitMesh(RefLucy<Mesh> mesh, const glm::mat4& entityTransform) {
		Submit([this, mesh, entityTransform]() {
			m_MeshDrawCommands.push_back(MeshDrawCommand(mesh, entityTransform));
		});
	}

	void OpenGLRenderer::Dispatch() {
		for (Func func : m_RenderFunctions) {
			func();
		}
		m_RenderFunctions.clear();
	}

	void OpenGLRenderer::Destroy() {
		for (RefLucy<Shader>& shader : m_ShaderLibrary.m_Shaders) {
			shader->Destroy();
		}
		m_CameraUniformBuffer->Destroy();
		m_TextureSlotsUniformBuffer->Destroy();
		m_RenderContext->Destroy();
	}

	GLenum GetGLMode(Topology topology) {
		switch (topology) {
			case Topology::LINES:
				return GL_LINES;
				break;
			case Topology::POINTS:
				return GL_POINTS;
				break;
			case Topology::TRIANGLES:
				return GL_TRIANGLES;
				break;
		}
	}

	void OpenGLAPICommands::ClearColor(float r, float g, float b, float a) {
		glClearColor(r, g, b, a);
	}

	void OpenGLAPICommands::Clear(uint32_t bitField) {
		glClear(bitField);
	}

	void OpenGLAPICommands::ReadBuffer(uint32_t mode) {
		glReadBuffer(mode);
	}

	void OpenGLAPICommands::ReadPixels(uint32_t x, uint32_t y, uint32_t width, uint32_t height, float* pixelValueOutput) {
		glReadPixels(x, y, width, height, GL_RGB, GL_FLOAT, pixelValueOutput);
	}

	void OpenGLAPICommands::ReadBuffer(RefLucy<FrameBuffer> frameBuffer, uint32_t mode) {
		glNamedFramebufferReadBuffer(frameBuffer->GetID(), mode);
	}

	void OpenGLAPICommands::DrawElements(Topology topology, uint32_t count, uint32_t indices) {
		glDrawElements(GetGLMode(topology), count, GL_UNSIGNED_INT, (const void*)(indices * sizeof(uint32_t)));
	}

	void OpenGLAPICommands::DrawElementsBaseVertex(Topology topology, uint32_t count, uint32_t indices, int32_t basevertex) {
		glDrawElementsBaseVertex(GetGLMode(topology), count, GL_UNSIGNED_INT, (const void*)(indices * sizeof(uint32_t)), basevertex);
	}
}