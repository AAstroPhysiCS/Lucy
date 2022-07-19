#include "lypch.h"
#include "OpenGLRHI.h"

#include "Memory/Buffer/FrameBuffer.h"
#include "Memory/Buffer/RenderBuffer.h"
#include "Memory/Buffer/UniformBuffer.h"
#include "Renderer.h"

#include "OpenGLRenderPass.h"
#include "Memory/Buffer/OpenGL/OpenGLUniformBuffer.h"

#include "Utils/Utils.h"

#include "glad/glad.h"

namespace Lucy {

	OpenGLRHI::OpenGLRHI(RenderArchitecture renderArchitecture)
		: RHI(renderArchitecture) {
		m_RenderContext = RenderContext::Create(m_Architecture);
		m_RenderContext->PrintInfo();

		const auto& pbrShader = Renderer::GetShaderLibrary().GetShader("LucyPBR");
		const auto& idShader = Renderer::GetShaderLibrary().GetShader("LucyID");

		auto [width, height] = Utils::ReadAttributeFromIni("Viewport", "Size");
		m_ViewportWidth = width;
		m_ViewportHeight = height;

		const uint32_t TargetSamples = 4;

		/*
		//------------ Main Framebuffer ------------
		{
			RenderBufferCreateInfo renderBufferSpecs;
			renderBufferSpecs.Attachment = GL_DEPTH_STENCIL_ATTACHMENT;
			renderBufferSpecs.InternalFormat = GL_DEPTH24_STENCIL8;
			renderBufferSpecs.Width = width;
			renderBufferSpecs.Height = height;
			renderBufferSpecs.Samples = TargetSamples;

			ImageCreateInfo textureAntialiased;
			textureAntialiased.Width = width;
			textureAntialiased.Height = height;
			textureAntialiased.Samples = TargetSamples;
			textureAntialiased.AttachmentIndex = 0;
			textureAntialiased.Format = { GL_RGBA8, GL_RGBA };
			textureAntialiased.PixelType = PixelType::UnsignedByte;

			ImageCreateInfo finalTextureSpec;
			finalTextureSpec.Width = width;
			finalTextureSpec.Height = height;
			finalTextureSpec.GenerateMipmap = true;
			finalTextureSpec.AttachmentIndex = 0;
			finalTextureSpec.Parameter.Min = GL_LINEAR;
			finalTextureSpec.Parameter.Mag = GL_LINEAR;
			finalTextureSpec.Format = { GL_RGBA8, GL_RGBA };
			finalTextureSpec.PixelType = PixelType::UnsignedByte;

			OpenGLFrameBufferSpecification geometryFrameBufferSpecs;
			geometryFrameBufferSpecs.ContentInfo.MultiSampled = true;
			geometryFrameBufferSpecs.ContentInfo.Width = width;
			geometryFrameBufferSpecs.ContentInfo.Height = height;
			geometryFrameBufferSpecs.RenderBuffer = RenderBuffer::Create(renderBufferSpecs);
			geometryFrameBufferSpecs.ContentInfo.TextureCreateInfos.push_back(textureAntialiased);
			geometryFrameBufferSpecs.BlittedTextureCreateInfo = finalTextureSpec;

			PipelineCreateInfo geometryPipelineSpecs;
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

			OpenGLRenderPassSpecification geometryPassSpecs;
			geometryPassSpecs.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

			geometryPipelineSpecs.FrameBuffer = FrameBuffer::Create(geometryFrameBufferSpecs);
			geometryPipelineSpecs.RenderPass = RenderPass::Create(geometryPassSpecs);
			geometryPipelineSpecs.Shader = pbrShader;
			m_GeometryPipeline = As(Pipeline::Create(geometryPipelineSpecs), OpenGLPipeline);

			//------------ Mouse Picking ------------
			ImageCreateInfo idTextureRGBSpecs;
			idTextureRGBSpecs.Width = width;
			idTextureRGBSpecs.Height = height;
			idTextureRGBSpecs.AttachmentIndex = 0;
			idTextureRGBSpecs.Parameter.Min = GL_NEAREST;
			idTextureRGBSpecs.Parameter.Mag = GL_NEAREST;
			idTextureRGBSpecs.Format = { GL_RGB32F , GL_RGB };
			idTextureRGBSpecs.PixelType = PixelType::Float;

			ImageCreateInfo idTextureDepthSpecs;
			idTextureDepthSpecs.Width = width;
			idTextureDepthSpecs.Height = height;
			idTextureDepthSpecs.AttachmentIndex = 1;
			idTextureDepthSpecs.Parameter.Min = GL_NEAREST;
			idTextureDepthSpecs.Parameter.Mag = GL_NEAREST;
			idTextureDepthSpecs.Format = { GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT };
			idTextureDepthSpecs.PixelType = PixelType::Float;

			OpenGLFrameBufferSpecification idFrameBufferSpecs;
			idFrameBufferSpecs.ContentInfo.Width = width;
			idFrameBufferSpecs.ContentInfo.Height = height;
			idFrameBufferSpecs.ContentInfo.TextureCreateInfos.push_back(idTextureRGBSpecs);
			idFrameBufferSpecs.ContentInfo.TextureCreateInfos.push_back(idTextureDepthSpecs);
			idFrameBufferSpecs.IsStorage = true;

			OpenGLRenderPassSpecification idRenderPassSpecs;
			idRenderPassSpecs.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

			geometryPipelineSpecs.FrameBuffer = FrameBuffer::Create(idFrameBufferSpecs);
			geometryPipelineSpecs.RenderPass = RenderPass::Create<OpenGLRenderPassSpecification>(idRenderPassSpecs);
			geometryPipelineSpecs.Shader = idShader;
			m_IDPipeline = As(Pipeline::Create(geometryPipelineSpecs), OpenGLPipeline);
		}

		Dispatch(); //just for init functions (if any)
		*/
	}

	void OpenGLRHI::Init() {
		s_CommandQueue.Init();
	}

	void OpenGLRHI::OnWindowResize() {
	}

	void OpenGLRHI::OnViewportResize() {
		//As(m_GeometryPipeline->GetFrameBuffer(), OpenGLFrameBuffer)->Resize(sizeX, sizeY);
		//As(m_IDPipeline->GetFrameBuffer(), OpenGLFrameBuffer)->Resize(sizeX, sizeY);
		//m_ViewportWidth = sizeX;
		//m_ViewportHeight = sizeY;
	}

	Entity OpenGLRHI::OnMousePicking() {
		/*
		m_IDPipeline->GetFrameBuffer().As<OpenGLFrameBuffer>()->Bind();
		glm::vec4 meshID;
		OpenGLAPICommands::ReadBuffer(GL_COLOR_ATTACHMENT0);
		OpenGLAPICommands::ReadPixels(m_ViewportMouseX, m_ViewportHeight - m_ViewportMouseY, 1, 1, (float*)&meshID);

		//checking if the data that is being read make sense
		if ((meshID.x > 255.0f || meshID.x < 0.0f) ||
			(meshID.y > 255.0f || meshID.y < 0.0f) ||
			(meshID.z > 255.0f || meshID.z < 0.0f) ||
			(meshID.w > 255.0f || meshID.w < 0.0f))
			return {};

		//checking if we clicked on the void
		if (meshID.x == 0 && meshID.y == 0 && meshID.z == 0 && meshID.w == 0)
			return {};

		Entity selectedEntity = m_ActiveScene->GetEntityByMeshID(meshID);
		m_IDPipeline->GetFrameBuffer().As<OpenGLFrameBuffer>()->Unbind();

		return selectedEntity;
		*/
		return {};
	}

	void OpenGLRHI::GeometryPass() {
		/*
		m_GeometryPipeline->Bind({});
		auto& uniformBuffers = m_GeometryPipeline->GetUniformBuffers<OpenGLUniformBuffer>(0);
		for (MeshDrawCommand meshComponent : m_StaticMeshDrawCommandQueue) {
			const Ref<Mesh>& mesh = meshComponent.Mesh;
			const std::vector<Ref<Material>>& materials = mesh->GetMaterials();
			std::vector<Submesh>& submeshes = mesh->GetSubmeshes();

			for (uint32_t i = 0; i < submeshes.size(); i++) {
				Submesh& submesh = submeshes[i];
				const Ref<Material> material = materials[submesh.MaterialIndex];
				const Ref<Shader>& shader = material->GetShader();
				//TODO: Mesh bind

				material->Bind(m_GeometryPipeline);
				uniformBuffers->SetData((void*)&(meshComponent.EntityTransform * submesh.Transform), sizeof(glm::mat4), sizeof(glm::mat4) * 2);
				OpenGLAPICommands::DrawElementsBaseVertex(m_GeometryPipeline->GetTopology(), submesh.IndexCount, submesh.BaseIndexCount, submesh.BaseVertexCount);
				material->Unbind(m_GeometryPipeline);
			}
		}
		m_GeometryPipeline->Unbind();
		*/
	}

	void OpenGLRHI::IDPass() {
		/*
		m_GeometryPipeline->Bind({});
		Ref<Shader> idShader = m_GeometryPipeline->GetShader();
		auto& uniformBuffers = m_GeometryPipeline->GetUniformBuffers<OpenGLUniformBuffer>(0);

		idShader->Bind();
		for (MeshDrawCommand meshComponent : m_StaticMeshDrawCommandQueue) {
			const Ref<Mesh>& mesh = meshComponent.Mesh;
			std::vector<Submesh>& submeshes = mesh->GetSubmeshes();

			//TODO: Mesh bind
			for (uint32_t i = 0; i < submeshes.size(); i++) {
				Submesh& submesh = submeshes[i];
				uniformBuffers->SetData((void*)&(meshComponent.EntityTransform * submesh.Transform), sizeof(glm::mat4), sizeof(glm::mat4) * 2);
				OpenGLAPICommands::DrawElementsBaseVertex(m_IDPipeline->GetTopology(), submesh.IndexCount, submesh.BaseIndexCount, submesh.BaseVertexCount);
			}
		}
		idShader->Unbind();
		m_GeometryPipeline->Unbind();
		*/
	}

	void OpenGLRHI::BeginScene(Scene& scene) {
		EditorCamera& camera = scene.GetEditorCamera();
		camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
		camera.Update();

		auto& uniformBuffers = m_GeometryPipeline->GetUniformBuffers<OpenGLUniformBuffer>("Camera");
		uniformBuffers->SetData((void*)&camera.GetViewMatrix(), sizeof(glm::mat4), 0);
		uniformBuffers->SetData((void*)&camera.GetProjectionMatrix(), sizeof(glm::mat4), sizeof(glm::mat4));
	}

	void OpenGLRHI::RenderScene() {
		GeometryPass();
		IDPass();
		//glfwSwapBuffers();
	}

	PresentResult OpenGLRHI::EndScene() {
		GLenum state = glGetError();
		if (state != GL_NO_ERROR) {
			Logger::Log(LoggerInfo::LUCY_CRITICAL, state);
			return PresentResult::ERROR_VALIDATION_FAILED_EXT;
		}
		return PresentResult::SUCCESS;
	}

	void OpenGLRHI::Enqueue(const SubmitFunc&& func) {
		m_RenderFunctionQueue.push_back(func);
	}

	void OpenGLRHI::EnqueueStaticMesh(Priority priority, Ref<Mesh> mesh, const glm::mat4& entityTransform) {
		Enqueue([=]() {
			m_StaticMeshDrawCommands.push_back(Memory::CreateRef<MeshDrawCommand>(priority, mesh, entityTransform));
		});
	}

	void OpenGLRHI::RecordStaticMeshToCommandQueue(Ref<Pipeline> pipeline, RecordFunc<Ref<DrawCommand>>&& func) {

	}

	void OpenGLRHI::BindBuffers(Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer) {
		vertexBuffer->Bind({});
		indexBuffer->Bind({});
	}

	void OpenGLRHI::Dispatch() {
		for (SubmitFunc func : m_RenderFunctionQueue) {
			func();
		}
		m_RenderFunctionQueue.clear();
	}

	void OpenGLRHI::Destroy() {
		//m_GeometryPipeline->DestroyUniformBuffers();
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

	void OpenGLAPICommands::ReadPixels(uint32_t x, uint32_t y, uint32_t width, uint32_t height, float* meshIDOutput) {
		glReadPixels(x, y, width, height, GL_RGB, GL_FLOAT, meshIDOutput);
	}

	void OpenGLAPICommands::ReadBuffer(Ref<OpenGLFrameBuffer> frameBuffer, uint32_t mode) {
		glNamedFramebufferReadBuffer(frameBuffer->GetID(), mode);
	}

	void OpenGLAPICommands::DrawElements(Topology topology, uint32_t count, uint32_t indices) {
		glDrawElements(GetGLMode(topology), count, GL_UNSIGNED_INT, (const void*)(indices * sizeof(uint32_t)));
	}

	void OpenGLAPICommands::DrawElementsBaseVertex(Topology topology, uint32_t count, uint32_t indices, int32_t basevertex) {
		glDrawElementsBaseVertex(GetGLMode(topology), count, GL_UNSIGNED_INT, (const void*)(indices * sizeof(uint32_t)), basevertex);
	}
}