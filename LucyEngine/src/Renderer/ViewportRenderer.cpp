#include "lypch.h"
#include "ViewportRenderer.h"

#include "Renderer.h"
#include "Scene/Components.h"

#include "Context/RHI.h"
#include "Context/VulkanPipeline.h"
#include "Buffer/Vulkan/VulkanFrameBuffer.h"

#include "Buffer/Vulkan/VulkanVertexBuffer.h" //TODO: Delete
#include "Buffer/Vulkan/VulkanIndexBuffer.h" //TODO: Delete
#include "Buffer/Vulkan/VulkanUniformBuffer.h" //TODO: Delete
#include "Image/Image.h" //TODO: Delete

#include "Synchronization/SynchItems.h"
#include "Utils.h"

namespace Lucy {

	RefLucy<VertexBuffer> vertexBuffer = nullptr; //TODO: Delete
	RefLucy<IndexBuffer> indexBuffer = nullptr; //TODO: Delete

	void ViewportRenderer::Init() {
		auto& vulkanTestShader = Renderer::GetShaderLibrary().GetShader("LucyVulkanTest");
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		RenderArchitecture rhi = Renderer::GetCurrentRenderArchitecture();

		auto [width, height] = Utils::ReadSizeFromIni("Viewport");
		Renderer::SetViewportSize(width, height);

		std::vector<ShaderLayoutElement> vertexLayout = {
				{ "a_Pos", ShaderDataSize::Float2 },
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
		geometryPipelineSpecs.Rasterization = { true, VK_CULL_MODE_BACK_BIT, 1.0f, PolygonMode::FILL };
		geometryPipelineSpecs.Shader = vulkanTestShader;

		if (rhi == RenderArchitecture::Vulkan) {
			RefLucy<VulkanRHIRenderPassDesc> vulkanRenderPassDesc = CreateRef<VulkanRHIRenderPassDesc>();
			vulkanRenderPassDesc->AttachmentReferences.push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
			vulkanRenderPassDesc->Descriptor.Format = VK_FORMAT_B8G8R8A8_UNORM;
			vulkanRenderPassDesc->Descriptor.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			vulkanRenderPassDesc->Descriptor.StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
			vulkanRenderPassDesc->Descriptor.StencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			vulkanRenderPassDesc->Descriptor.StencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			vulkanRenderPassDesc->Descriptor.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			vulkanRenderPassDesc->Descriptor.FinalLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR;

			geometryPassSpecs.InternalInfo = vulkanRenderPassDesc;

			RefLucy<VulkanRHIImageDesc> vulkanTextureDesc = CreateRef<VulkanRHIImageDesc>();
			vulkanTextureDesc->GenerateSampler = true;
			vulkanTextureDesc->ImGuiUsage = true;

			geometryTextureSpecification.InternalInfo = vulkanTextureDesc;

			RefLucy<VulkanRHIFrameBufferDesc> vulkanFrameBufferDesc = CreateRef<VulkanRHIFrameBufferDesc>();
			vulkanFrameBufferDesc->ImageBuffers.reserve(swapChain.GetImageCount());
			for (uint32_t i = 0; i < swapChain.GetImageCount(); i++)
				vulkanFrameBufferDesc->ImageBuffers.emplace_back(As(Image2D::Create(geometryTextureSpecification), VulkanImage2D));
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

			RefLucy<VulkanRHIRenderPassDesc> vulkanRenderPassDesc = CreateRef<VulkanRHIRenderPassDesc>();
			vulkanRenderPassDesc->AttachmentReferences.push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
			vulkanRenderPassDesc->Descriptor.Format = swapChain.GetSurfaceFormat().format;
			vulkanRenderPassDesc->Descriptor.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			vulkanRenderPassDesc->Descriptor.StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
			vulkanRenderPassDesc->Descriptor.StencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			vulkanRenderPassDesc->Descriptor.StencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			vulkanRenderPassDesc->Descriptor.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			vulkanRenderPassDesc->Descriptor.FinalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			uiRenderPassSpecs.InternalInfo = vulkanRenderPassDesc;
			s_ImGuiPipeline.UIRenderPass = RenderPass::Create(uiRenderPassSpecs);

			FrameBufferSpecification uiFrameBufferSpecs;
			uiFrameBufferSpecs.Width = swapChain.GetExtent().width;
			uiFrameBufferSpecs.Height = swapChain.GetExtent().height;

			RefLucy<VulkanRHIFrameBufferDesc> vulkanFrameBufferDesc = CreateRef<VulkanRHIFrameBufferDesc>();
			vulkanFrameBufferDesc->ImageViews = swapChain.GetImageViews();
			vulkanFrameBufferDesc->RenderPass = s_ImGuiPipeline.UIRenderPass;

			uiFrameBufferSpecs.InternalInfo = vulkanFrameBufferDesc;
			s_ImGuiPipeline.UIFramebuffer = FrameBuffer::Create(uiFrameBufferSpecs);
		}

		vertexBuffer = VertexBuffer::Create(16);
		indexBuffer = IndexBuffer::Create(6);
		vertexBuffer->AddData(
			{ -0.5f, -0.5f, 1.0f, 0.0f,
			   0.5f, -0.5f, 0.0f, 0.0f,
			   0.5f, 0.5f, 0.0f, 1.0f,
			  -0.5f, 0.5f, 1.0f, 1.0f
			}
		);
		vertexBuffer->Load();

		indexBuffer->AddData(
			{ 0, 1, 2, 2, 3, 0 }
		);
		indexBuffer->Load();
	}

	void ViewportRenderer::Begin(Scene& scene) {
		Renderer::Dispatch(); //dispatch all the functions that should happen on the main render thread (before the passes)
		Renderer::BeginScene(scene);
	}

	void ViewportRenderer::Dispatch() {
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
		Renderer::OnViewportResize();
	}

	void ViewportRenderer::Destroy() {
		vertexBuffer->Destroy(); //TODO: Delete
		indexBuffer->Destroy(); //TODO: Delete

		s_GeometryPipeline->Destroy();
		s_ImGuiPipeline.UIFramebuffer->Destroy();
		s_ImGuiPipeline.UIRenderPass->Destroy();
	}

	void ViewportRenderer::GeometryPass() {
		auto& uniformBuffer = s_GeometryPipeline->GetUniformBuffers<VulkanUniformBuffer>(0);

		struct MVP {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
		};

		auto& extent = VulkanSwapChain::Get().GetExtent();

		MVP mvp;
		mvp.model = glm::rotate(glm::mat4(1.0f), 0.5f * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		mvp.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		mvp.proj = glm::perspective(glm::radians(45.0f), (float)extent.width / extent.height, 0.1f, 10.0f);
		mvp.proj[1][1] *= -1;

		uniformBuffer->SetData((void*)&mvp, sizeof(MVP), 0);
		uniformBuffer->Update();

		//TODO: this function should work in parallel, meaning it should be multithreaded...
		Renderer::RecordToCommandQueue([]() {
			Renderer::BindPipeline(s_GeometryPipeline);
			Renderer::BindBuffers(vertexBuffer, indexBuffer);
			Renderer::DrawIndexed(6, 1, 0, 0, 0);
			Renderer::UnbindPipeline();
			/*
			const RefLucy<Mesh>& mesh = drawCommand.Mesh;
			const glm::mat4& entityTransform = drawCommand.EntityTransform;

			auto& uniformBuffers = s_GeometryPipeline->GetUniformBuffers<VulkanUniformBuffer>(1);
			const std::vector<RefLucy<Material>>& materials = mesh->GetMaterials();
			std::vector<Submesh>& submeshes = mesh->GetSubmeshes();

			for (uint32_t i = 0; i < submeshes.size(); i++) {
				Submesh& submesh = submeshes[i];
				const RefLucy<Material> material = materials[submesh.MaterialIndex];
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
