#include "lypch.h"
#include "ViewportRenderer.h"

#include "Renderer.h"
#include "Scene/Components.h"

#include "Context/RHI.h"
#include "Context/VulkanPipeline.h"
#include "Memory/Buffer/Vulkan/VulkanFrameBuffer.h"

#include "Memory/Buffer/Vulkan/VulkanVertexBuffer.h" //TODO: Delete
#include "Memory/Buffer/Vulkan/VulkanIndexBuffer.h" //TODO: Delete
#include "Memory/Buffer/Vulkan/VulkanUniformBuffer.h" //TODO: Delete
#include "Image/Image.h" //TODO: Delete

#include "Synchronization/SynchItems.h"
#include "Utils.h"

namespace Lucy {

	Ref<VertexBuffer> vertexBuffer = nullptr; //TODO: Delete
	Ref<IndexBuffer> indexBuffer = nullptr; //TODO: Delete

	void ViewportRenderer::Init() {
		auto& vulkanTestShader = Renderer::GetShaderLibrary().GetShader("LucyVulkanTest");
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		RenderArchitecture rhi = Renderer::GetCurrentRenderArchitecture();

		auto [width, height] = Utils::ReadViewportSizeFromIni("Viewport");
		Renderer::SetViewportSize(width, height);

		std::vector<ShaderLayoutElement> vertexLayout = {
				{ "a_Pos", ShaderDataSize::Float3 },
				{ "a_TextureCoords", ShaderDataSize::Float2 },
				//{ "a_ID", ShaderDataSize::Float3 },
				//{ "a_Normals", ShaderDataSize::Float3 },
				//{ "a_Tangents", ShaderDataSize::Float3 },
				//{ "a_BiTangents", ShaderDataSize::Float3 }
		};

		RenderPassSpecification geometryPassSpecs;
		geometryPassSpecs.ClearColor = { 0.0f, 0.0f, 0.5f, 1.0f };

		ImageSpecification geometryTextureSpecification;
		geometryTextureSpecification.Width = width;
		geometryTextureSpecification.Height = height;
		geometryTextureSpecification.Format = VK_FORMAT_B8G8R8A8_UNORM;
		geometryTextureSpecification.ImageType = ImageType::Type2D;
		geometryTextureSpecification.Parameter.Mag = VK_FILTER_LINEAR;
		geometryTextureSpecification.Parameter.Min = VK_FILTER_LINEAR;
		geometryTextureSpecification.Parameter.U = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		geometryTextureSpecification.Parameter.V = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		geometryTextureSpecification.Parameter.W = VK_SAMPLER_ADDRESS_MODE_REPEAT;

		FrameBufferSpecification geometryFrameBufferSpecs;
		geometryFrameBufferSpecs.Width = width;
		geometryFrameBufferSpecs.Height = height;

		PipelineSpecification geometryPipelineSpecs;
		geometryPipelineSpecs.VertexShaderLayout = VertexShaderLayout(vertexLayout);
		geometryPipelineSpecs.Topology = Topology::TRIANGLES;
		geometryPipelineSpecs.Rasterization = { true, VK_CULL_MODE_NONE, 1.0f, PolygonMode::FILL };
		geometryPipelineSpecs.Shader = vulkanTestShader;

		if (rhi == RenderArchitecture::Vulkan) {
			Ref<VulkanRHIRenderPassDesc> vulkanRenderPassDesc = Memory::CreateRef<VulkanRHIRenderPassDesc>();
			vulkanRenderPassDesc->ColorAttachments.push_back(
				{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }
			);
			vulkanRenderPassDesc->DepthEnable = true; //enables the support for depth buffer
			vulkanRenderPassDesc->ColorDescriptor.Format = VK_FORMAT_B8G8R8A8_UNORM;
			vulkanRenderPassDesc->ColorDescriptor.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			vulkanRenderPassDesc->ColorDescriptor.StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
			vulkanRenderPassDesc->ColorDescriptor.StencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			vulkanRenderPassDesc->ColorDescriptor.StencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			vulkanRenderPassDesc->ColorDescriptor.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			vulkanRenderPassDesc->ColorDescriptor.FinalLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR;
			geometryPassSpecs.InternalInfo = vulkanRenderPassDesc;

			Ref<VulkanRHIImageDesc> vulkanTextureDesc = Memory::CreateRef<VulkanRHIImageDesc>();
			vulkanTextureDesc->GenerateSampler = true;
			vulkanTextureDesc->ImGuiUsage = true;

			geometryTextureSpecification.InternalInfo = vulkanTextureDesc;

			Ref<VulkanRHIFrameBufferDesc> vulkanFrameBufferDesc = Memory::CreateRef<VulkanRHIFrameBufferDesc>();
			vulkanFrameBufferDesc->ImageBuffers.reserve(swapChain.GetImageCount());
			for (uint32_t i = 0; i < swapChain.GetImageCount(); i++)
				vulkanFrameBufferDesc->ImageBuffers.emplace_back(Image2D::Create(geometryTextureSpecification).As<VulkanImage2D>());
			vulkanFrameBufferDesc->RenderPass = RenderPass::Create(geometryPassSpecs);

			geometryFrameBufferSpecs.InternalInfo = vulkanFrameBufferDesc;
			geometryPipelineSpecs.RenderPass = vulkanFrameBufferDesc->RenderPass;
		}

		geometryPipelineSpecs.FrameBuffer = FrameBuffer::Create(geometryFrameBufferSpecs);
		s_GeometryPipeline = Pipeline::Create(geometryPipelineSpecs);

		/*
		----ImGui (for Vulkan; does not need a separate pipeline)----
		*/

		if (rhi == RenderArchitecture::Vulkan) {
			RenderPassSpecification uiRenderPassSpecs;
			uiRenderPassSpecs.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

			Ref<VulkanRHIRenderPassDesc> vulkanRenderPassDesc = Memory::CreateRef<VulkanRHIRenderPassDesc>();
			vulkanRenderPassDesc->ColorAttachments.push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
			vulkanRenderPassDesc->ColorDescriptor.Format = swapChain.GetSurfaceFormat().format;
			vulkanRenderPassDesc->ColorDescriptor.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			vulkanRenderPassDesc->ColorDescriptor.StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
			vulkanRenderPassDesc->ColorDescriptor.StencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			vulkanRenderPassDesc->ColorDescriptor.StencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			vulkanRenderPassDesc->ColorDescriptor.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			vulkanRenderPassDesc->ColorDescriptor.FinalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			uiRenderPassSpecs.InternalInfo = vulkanRenderPassDesc;
			s_ImGuiPipeline.UIRenderPass = RenderPass::Create(uiRenderPassSpecs);

			FrameBufferSpecification uiFrameBufferSpecs;
			uiFrameBufferSpecs.Width = swapChain.GetExtent().width;
			uiFrameBufferSpecs.Height = swapChain.GetExtent().height;

			Ref<VulkanRHIFrameBufferDesc> vulkanFrameBufferDesc = Memory::CreateRef<VulkanRHIFrameBufferDesc>();
			vulkanFrameBufferDesc->ImageViews = swapChain.GetImageViews();
			vulkanFrameBufferDesc->RenderPass = s_ImGuiPipeline.UIRenderPass;

			uiFrameBufferSpecs.InternalInfo = vulkanFrameBufferDesc;
			s_ImGuiPipeline.UIFramebuffer = FrameBuffer::Create(uiFrameBufferSpecs);
		}

		vertexBuffer = VertexBuffer::Create(40);
		indexBuffer = IndexBuffer::Create(12);
		vertexBuffer->SetData(
			{ 
				-0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
				0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
				0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
				-0.5f, 0.5f, 0.0f, 1.0f, 1.0f,

				-0.5f, -0.5f, -0.5f, 1.0f, 0.0f,
				0.5f, -0.5f, -0.5f, 0.0f, 0.0f,
				0.5f, 0.5f, -0.5f, 0.0f, 1.0f,
				-0.5f, 0.5f, -0.5f, 1.0f, 1.0f
			}
		);
		vertexBuffer->LoadToGPU();

		indexBuffer->SetData(
			{ 
				0, 1, 2, 2, 3, 0,
				4, 5, 6, 6, 7, 4
			}
		);
		indexBuffer->LoadToGPU();
	}

	void ViewportRenderer::Begin(Scene& scene) {
		Renderer::Dispatch(); //dispatch all the functions that should happen on the main render thread (before the passes)
		Renderer::BeginScene(scene);
	}

	void ViewportRenderer::Dispatch(Scene& scene) {
		auto& uniformBuffer = s_GeometryPipeline->GetUniformBuffers<VulkanUniformBuffer>(0);

		EditorCamera& camera = scene.GetEditorCamera();
		auto mvp = camera.GetMVP();

		uniformBuffer->SetData((void*)&mvp, sizeof(mvp), 0);
		uniformBuffer->Update();

		IDPass();
		GeometryPass();
		UIPass();

		Renderer::RenderScene();
	}

	void ViewportRenderer::End() {
		PresentResult result = Renderer::EndScene();
		if (result == PresentResult::ERROR_OUT_OF_DATE_KHR || result == PresentResult::SUBOPTIMAL_KHR)
			OnWindowResize();
		Renderer::ClearQueues();
	}

	void ViewportRenderer::OnWindowResize() {
		Renderer::OnWindowResize();
	}

	void ViewportRenderer::Destroy() {
		vertexBuffer->DestroyHandle(); //TODO: Delete
		indexBuffer->DestroyHandle(); //TODO: Delete

		s_GeometryPipeline->Destroy();
		s_ImGuiPipeline.UIFramebuffer->Destroy();
		s_ImGuiPipeline.UIRenderPass->Destroy();
	}

	void ViewportRenderer::GeometryPass() {
		
		//TODO: this function should work in parallel, meaning it should be multithreaded...
		Renderer::RecordToCommandQueue([]() {
			Renderer::BindPipeline(s_GeometryPipeline);
			Renderer::BindBuffers(vertexBuffer, indexBuffer);
			Renderer::DrawIndexed(12, 1, 0, 0, 0);
			Renderer::UnbindPipeline();
			/*
			const Ref<Mesh>& mesh = drawCommand.Mesh;
			const glm::mat4& entityTransform = drawCommand.EntityTransform;

			auto& uniformBuffers = s_GeometryPipeline->GetUniformBuffers<VulkanUniformBuffer>(1);
			const std::vector<Ref<Material>>& materials = mesh->GetMaterials();
			std::vector<Submesh>& submeshes = mesh->GetSubmeshes();

			for (uint32_t i = 0; i < submeshes.size(); i++) {
				Submesh& submesh = submeshes[i];
				const Ref<Material> material = materials[submesh.MaterialIndex];
				const glm::mat4& entityTransform = submesh.Transform;

				material->Bind(s_GeometryPipeline);
				uniformBuffers->SetData((void*)&(entityTransform * submesh.Transform), sizeof(glm::mat4), sizeof(glm::mat4) * 2);
				Renderer::DrawIndexed(submesh.IndexCount, 0, 0, submesh.BaseIndexCount, submesh.BaseVertexCount);
				material->Unbind(s_GeometryPipeline);
			}
			*/
		});
	}

	void ViewportRenderer::IDPass() {
		//TODO: later
	}

	void ViewportRenderer::UIPass() {
		Renderer::UIPass(s_ImGuiPipeline);
	}
}
