#include "lypch.h"

#include "Renderer.h"
#include "Context/RendererAPI.h"

#include "Buffer/FrameBuffer.h"
#include "Buffer/RenderBuffer.h"
#include "Buffer/OpenGL/OpenGLFrameBuffer.h"

#include "Shader/Shader.h"
#include "Texture/Texture.h"

#include "RenderPass.h"
#include "RenderCommand.h"

#include "Core/Input.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "../Utils.h"

#include <iostream>
#include "glad/glad.h"

namespace Lucy {

	RefLucy<RendererAPI> Renderer::s_RendererAPI;
	RefLucy<RenderContext> Renderer::s_RenderContext;
	RefLucy<RenderPass> Renderer::s_GeometryPass;
	RefLucy<RenderPass> Renderer::s_IDPass;
	RefLucy<Window> Renderer::s_Window;

	std::vector<Func> Renderer::s_RenderQueue;
	std::vector<MeshDrawCommand> Renderer::s_MeshDrawCommand;

	int32_t Renderer::s_ViewportWidth = 0;
	int32_t Renderer::s_ViewportHeight = 0;

	float Renderer::s_ViewportMouseX = 0;
	float Renderer::s_ViewportMouseY = 0;

	ShaderLibrary Renderer::s_ShaderLibrary;
	Scene* Renderer::s_ActiveScene = nullptr;
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
			Shader::Create("LucyID", "assets/shaders/LucyID.glsl");
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
			geometryPipelineSpecs.Rasterization = { true, GL_BACK, 1.0f, GL_FILL };

			RenderPassSpecification geometryPassSpecs;
			geometryPassSpecs.FrameBuffer = FrameBuffer::Create(geometryFrameBufferSpecs);
			geometryPassSpecs.Pipeline = Pipeline::Create(geometryPipelineSpecs);
			geometryPassSpecs.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
			s_GeometryPass = RenderPass::Create(geometryPassSpecs);

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
			idRenderPassSpecs.FrameBuffer = FrameBuffer::Create(idFrameBufferSpecs);
			idRenderPassSpecs.Pipeline = Pipeline::Create(geometryPipelineSpecs);
			idRenderPassSpecs.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
			s_IDPass = RenderPass::Create(idRenderPassSpecs);
		}

		Renderer::Dispatch(); //just for init functions

		int32_t textures[32];
		for (int32_t i = 0; i < 32; i++) textures[i] = i;

		RefLucy<Shader> pbrShader = s_ShaderLibrary.GetShader("LucyPBR");
		pbrShader->Bind();
		pbrShader->SetInt("u_Textures", textures, 32);
		pbrShader->Unbind();
	}

	void Renderer::Submit(const Func&& func) {
		s_RenderQueue.push_back(func);
	}

	void Renderer::SubmitMesh(RefLucy<Mesh> mesh, const glm::mat4& entityTransform) {
		Submit([mesh, entityTransform]() {
			s_MeshDrawCommand.push_back(MeshDrawCommand(mesh, entityTransform));
		});
	}

	void Renderer::OnFramebufferResize(float sizeX, float sizeY) {
		s_GeometryPass->GetFrameBuffer()->Resize(sizeX, sizeY);
		s_IDPass->GetFrameBuffer()->Resize(sizeX, sizeY);
		Renderer::SetViewportSize(sizeX, sizeY);
	}

	Entity Renderer::OnMousePicking() {
		s_IDPass->GetFrameBuffer()->Bind();
		glm::vec3 pixelValue;
		RenderCommand::ReadBuffer(GL_COLOR_ATTACHMENT0);
		RenderCommand::ReadPixels(s_ViewportMouseX, s_ViewportHeight - s_ViewportMouseY, 1, 1, (float*) &pixelValue);

		//checking if the data that is being read make sense
		if ((pixelValue.x > 255.0f || pixelValue.x < 0.0f) || 
			(pixelValue.y > 255.0f || pixelValue.y < 0.0f) ||
			(pixelValue.z > 255.0f || pixelValue.z < 0.0f))
			return {};

		//checking if we clicked on the void
		if (pixelValue.x == 0 && pixelValue.y == 0 && pixelValue.z == 0)
			return {};

		Entity selectedEntity = s_ActiveScene->GetEntityByPixelValue(pixelValue);
		s_IDPass->GetFrameBuffer()->Unbind();
		
		return selectedEntity;
	}

	void Renderer::GeometryPass() {
		RenderCommand::Begin(s_GeometryPass);
		for (MeshDrawCommand meshComponent : s_MeshDrawCommand) {
			RefLucy<Mesh> mesh = meshComponent.Mesh;
			std::vector<Material>& materials = mesh->GetMaterials();
			std::vector<Submesh>& submeshes = mesh->GetSubmeshes();

			mesh->Bind();
			for (uint32_t i = 0; i < submeshes.size(); i++) {
				Submesh& submesh = submeshes[i];
				Material& material = materials[submesh.MaterialIndex];
				RefLucy<Shader> shader = material.GetShader();

				material.Bind();
				shader->SetMat4("u_ModelMatrix", meshComponent.EntityTransform * submesh.Transform);
				shader->SetMat4("u_ViewMatrix", s_ActiveCamera->GetViewMatrix());
				shader->SetMat4("u_ProjMatrix", s_ActiveCamera->GetProjectionMatrix());
				RenderCommand::DrawElementsBaseVertex(submesh.IndexCount, submesh.BaseIndexCount, submesh.BaseVertexCount);
				material.Unbind();
			}
			mesh->Unbind();
		}
		RenderCommand::End(s_GeometryPass);
	}

	void Renderer::IDPass() {
		RefLucy<Shader> idShader = s_ShaderLibrary.GetShader("LucyID");
		RenderCommand::Begin(s_IDPass);

		idShader->Bind();
		idShader->SetMat4("u_ViewMatrix", s_ActiveCamera->GetViewMatrix());
		idShader->SetMat4("u_ProjMatrix", s_ActiveCamera->GetProjectionMatrix());
		for (MeshDrawCommand meshComponent : s_MeshDrawCommand) {
			RefLucy<Mesh> mesh = meshComponent.Mesh;
			std::vector<Submesh>& submeshes = mesh->GetSubmeshes();

			mesh->Bind();
			for (uint32_t i = 0; i < submeshes.size(); i++) {
				Submesh& submesh = submeshes[i];
				idShader->SetMat4("u_ModelMatrix", meshComponent.EntityTransform * submesh.Transform);
				RenderCommand::DrawElementsBaseVertex(submesh.IndexCount, submesh.BaseIndexCount, submesh.BaseVertexCount);
			}
			mesh->Unbind();
		}
		idShader->Unbind();
		RenderCommand::End(s_IDPass);
	}

	void Renderer::BeginScene(Scene& scene) {
		s_Window->Update();

		EditorCamera& camera = scene.GetEditorCamera();
		camera.SetViewportSize(s_ViewportWidth, s_ViewportHeight);
		camera.Update();
		s_ActiveCamera = &camera;
		s_ActiveScene = &scene;
	}

	void Renderer::EndScene() {
		GeometryPass();
		IDPass();
	}

	void Renderer::Dispatch() {
		for (Func func : s_RenderQueue) {
			func();
		}
		s_RenderQueue.clear();
	}

	void Renderer::SetViewportSize(int32_t width, int32_t height) {
		s_ViewportWidth = width;
		s_ViewportHeight = height;
	}

	void Renderer::SetViewportMousePosition(float x, float y) {
		s_ViewportMouseX = x;
		s_ViewportMouseY = y;
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