#include "lypch.h"

#include "Renderer.h"

#include "Context/RendererAPI.h"
#include "Buffer/FrameBuffer.h"
#include "Buffer/RenderBuffer.h"

#include "Buffer/OpenGL/OpenGLFrameBuffer.h"

#include "Texture/Texture.h"
#include "../Utils.h"

#include "Shader/Shader.h"
#include "RenderPass.h"
#include "RenderCommand.h"

#include <iostream>
#include "glad/glad.h"

namespace Lucy {

	RefLucy<RendererAPI> Renderer::s_RendererAPI;
	RefLucy<RenderContext> Renderer::s_RenderContext;
	RefLucy<RenderPass> Renderer::s_GeometryPass;
	RefLucy<Window> Renderer::s_Window;

	std::vector<Func> Renderer::s_RenderQueue;
	std::vector<MeshDrawCommand> Renderer::s_MeshDrawCommand;

	int32_t Renderer::m_ViewportWidth = 0;
	int32_t Renderer::m_ViewportHeight = 0;

	ShaderLibrary Renderer::s_ShaderLibrary;
	Camera* Renderer::s_ActiveCamera = nullptr;

	void Renderer::Init(RefLucy<Window> window, RenderAPI renderContext) {
		s_RenderContext = RenderContext::Create(renderContext);
		s_RenderContext->PrintInfo();

		s_RendererAPI = RendererAPI::Create();
		s_Window = window;

		auto [width, height] = Utils::ReadSizeFromIni("Viewport");

		//------------ Shaders ------------
		{
			Shader::Create("LucyPBR", "assets/shaders/LucyPBR.glsl");
		}

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
			textureAntialiased.Samples = TargetSamples;
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

			PipelineSpecification geometryPipelineSpecs;

			std::vector<ShaderLayoutElement> vertexLayout = {
					{ "a_Pos", ShaderDataSize::Float3 },
					{ "a_ID", ShaderDataSize::Float1 },
					{ "a_TextureCoords", ShaderDataSize::Float2 },
					{ "a_Normals", ShaderDataSize::Float3 },
					{ "a_Tangents", ShaderDataSize::Float3 },
					{ "a_BiTangents", ShaderDataSize::Float3 }
			};

			geometryPipelineSpecs.VertexShaderLayout = VertexShaderLayout(vertexLayout);
			geometryPipelineSpecs.Topology = Topology::TRIANGLES;
			geometryPipelineSpecs.Rasterization = { false, 1.0f, GL_FILL };

			RenderPassSpecification geometryPassSpecs;
			geometryPassSpecs.FrameBuffer = FrameBuffer::Create(frameBufferSpecs);
			geometryPassSpecs.Pipeline = Pipeline::Create(geometryPipelineSpecs);
			geometryPassSpecs.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
			s_GeometryPass = RenderPass::Create(geometryPassSpecs);
		}

		Renderer::Dispatch(); //just for init functions
	}

	void Renderer::Submit(const Func&& func) {
		s_RenderQueue.push_back(func);
	}

	void Renderer::SubmitMesh(RefLucy<Mesh>& mesh, const glm::mat4& entityTransform) {
		Submit([=]() {
			s_MeshDrawCommand.push_back(MeshDrawCommand(mesh, entityTransform));
		});
	}

	void Renderer::GeometryPass() {
		RenderCommand::Begin(s_GeometryPass);

		for (MeshDrawCommand meshComponent : s_MeshDrawCommand) {

			RefLucy<Mesh>& mesh = meshComponent.Mesh;

			std::vector<Material>& materials = mesh->GetMaterials();
			std::vector<Submesh>& submeshes = mesh->GetSubmeshes();

			mesh->Bind();

			for (uint32_t i = 0; i < submeshes.size(); i++) {
				Submesh& submesh = submeshes[i];
				Material& material = materials[submesh.MaterialIndex];
				RefLucy<Shader>& shader = material.GetShader();

				material.Bind();
				shader->SetMat4("u_ModelMatrix", meshComponent.EntityTransform * submesh.Transform);
				shader->SetMat4("u_ViewMatrix", s_ActiveCamera->GetViewMatrix());
				shader->SetMat4("u_ProjMatrix", s_ActiveCamera->GetProjectionMatrix());
				shader->SetInt("albedoSlot", 1);
				int32_t textures[32];
				for (int32_t i = 0; i < 32; i++) textures[i] = i;
				shader->SetInt("u_Textures", textures, 32);
				RenderCommand::DrawElementsBaseVertex(submesh.IndexCount, submesh.BaseIndexCount, submesh.BaseVertexCount);
				material.Unbind();
			}

			mesh->Unbind();
		}

		RenderCommand::End(s_GeometryPass);
	}

	void Renderer::BeginScene(Scene& scene) {

		s_Window->Update();

		EditorCamera& camera = scene.GetEditorCamera();
		camera.UpdateProjection();
		camera.UpdateView();

		s_ActiveCamera = &camera;
	}

	void Renderer::EndScene() {
		GeometryPass();
	}

	void Renderer::Dispatch() {
		for (Func func : s_RenderQueue) {
			func();
		}
		s_RenderQueue.clear();
	}

	void Renderer::SetViewportSize(int32_t width, int32_t height) {
		m_ViewportWidth = width;
		m_ViewportHeight = height;
	}

	void Renderer::ClearDrawCommands() {
		s_MeshDrawCommand.clear();
	}

	void Renderer::Destroy() {
		for (RefLucy<Shader> shader : s_ShaderLibrary.m_Shaders) {
			shader->Destroy();
		}
		glfwTerminate();
	}
}