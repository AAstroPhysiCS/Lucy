#include "lypch.h"
#include "OpenGLRenderer.h"

#include "Buffer/FrameBuffer.h"
#include "Buffer/RenderBuffer.h"
#include "Buffer/UniformBuffer.h"
#include "Mesh.h"

#include "OpenGLRenderCommand.h"
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

		m_RenderCommand = RenderCommand::Create();
		
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
		const auto& renderCommand = As(m_RenderCommand, OpenGLRenderCommand);
		renderCommand->ReadBuffer(GL_COLOR_ATTACHMENT0);
		renderCommand->ReadPixels(m_ViewportMouseX, m_ViewportHeight - m_ViewportMouseY, 1, 1, (float*)&pixelValue);

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
		const auto& renderCommand = As(m_RenderCommand, OpenGLRenderCommand);
		renderCommand->Begin(m_GeometryPipeline);
		for (MeshDrawCommand meshComponent : m_MeshDrawCommand) {
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
				renderCommand->DrawElementsBaseVertex(submesh.IndexCount, submesh.BaseIndexCount, submesh.BaseVertexCount);
				material.Unbind();
			}
			mesh->Unbind();
		}
		renderCommand->End(m_GeometryPipeline);
	}

	void OpenGLRenderer::IDPass() {
		RefLucy<Shader> idShader = m_ShaderLibrary.GetShader("LucyID");
		const auto& renderCommand = As(m_RenderCommand, OpenGLRenderCommand);
		renderCommand->Begin(m_IDPipeline);

		idShader->Bind();
		for (MeshDrawCommand meshComponent : m_MeshDrawCommand) {
			const RefLucy<Mesh>& mesh = meshComponent.Mesh;
			std::vector<Submesh>& submeshes = mesh->GetSubmeshes();

			mesh->Bind();
			for (uint32_t i = 0; i < submeshes.size(); i++) {
				Submesh& submesh = submeshes[i];
				m_CameraUniformBuffer->SetData((void*)&(meshComponent.EntityTransform * submesh.Transform), sizeof(glm::mat4), sizeof(glm::mat4) * 2);
				renderCommand->DrawElementsBaseVertex(submesh.IndexCount, submesh.BaseIndexCount, submesh.BaseVertexCount);
			}
			mesh->Unbind();
		}
		idShader->Unbind();
		renderCommand->End(m_IDPipeline);
	}

	void OpenGLRenderer::Draw() {
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
		Draw();
	}

	void OpenGLRenderer::ClearCommands() {
		m_MeshDrawCommand.clear();
	}

	void OpenGLRenderer::Submit(const Func&& func) {
		m_RenderQueue.push_back(func);
	}

	void OpenGLRenderer::SubmitMesh(RefLucy<Mesh> mesh, const glm::mat4& entityTransform) {
		Submit([this, mesh, entityTransform]() {
			m_MeshDrawCommand.push_back(MeshDrawCommand(mesh, entityTransform));
		});
	}

	void OpenGLRenderer::Dispatch() {
		for (Func func : m_RenderQueue) {
			func();
		}
		m_RenderQueue.clear();
	}

	void OpenGLRenderer::Destroy() {
		for (RefLucy<Shader>& shader : m_ShaderLibrary.m_Shaders) {
			shader->Destroy();
		}
		m_CameraUniformBuffer->Destroy();
		m_TextureSlotsUniformBuffer->Destroy();
		m_RenderContext->Destroy();
	}
}