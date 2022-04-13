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

#include "Synchronization/SynchItems.h"

namespace Lucy {

	VulkanVertexBuffer* vertexBuffer = nullptr; //TODO: Delete
	VulkanIndexBuffer* indexBuffer = nullptr; //TODO: Delete

	void ViewportRenderer::Init() {
		auto& vulkanTestShader = Renderer::GetShaderLibrary().GetShader("LucyVulkanTest");

		PipelineSpecification geometryPipelineSpecs;
		std::vector<ShaderLayoutElement> vertexLayout = {
				{ "a_Pos", ShaderDataSize::Float2 },
				//{ "a_ID", ShaderDataSize::Float3 },
				//{ "a_TextureCoords", ShaderDataSize::Float2 },
				//{ "a_Normals", ShaderDataSize::Float3 },
				//{ "a_Tangents", ShaderDataSize::Float3 },
				//{ "a_BiTangents", ShaderDataSize::Float3 }
		};

		RenderPassSpecification geometryPassSpecs;
		geometryPassSpecs.ClearColor = { 0.0f, 0.0f, 0.5f, 1.0f };
		geometryPassSpecs.AttachmentReferences = {
			{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }
		};
		geometryPassSpecs.Descriptor.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		geometryPassSpecs.Descriptor.StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		geometryPassSpecs.Descriptor.StencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		geometryPassSpecs.Descriptor.StencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		geometryPassSpecs.Descriptor.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		geometryPassSpecs.Descriptor.FinalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		
		FrameBufferSpecification geometryFrameBufferSpecs;
		geometryFrameBufferSpecs.ImageViews = swapChain.GetImageViews();
		geometryFrameBufferSpecs.Width = swapChain.GetExtent().width;
		geometryFrameBufferSpecs.Height = swapChain.GetExtent().height;

		geometryPipelineSpecs.VertexShaderLayout = VertexShaderLayout(vertexLayout);
		geometryPipelineSpecs.Topology = Topology::TRIANGLES;
		geometryPipelineSpecs.Rasterization = { true, VK_CULL_MODE_BACK_BIT, 1.0f, PolygonMode::FILL };
		geometryPipelineSpecs.Shader = vulkanTestShader;
		geometryPipelineSpecs.RenderPass = RenderPass::Create(geometryPassSpecs);
		geometryPipelineSpecs.FrameBuffer = FrameBuffer::Create(geometryFrameBufferSpecs, geometryPipelineSpecs.RenderPass);
		s_GeometryPipeline = Pipeline::Create(geometryPipelineSpecs);

		/*
		----ImGui (does not need a separate pipeline)----
		*/
		RenderPassSpecification uiRenderPassSpecs;
		uiRenderPassSpecs.AttachmentReferences.push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
		uiRenderPassSpecs.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		uiRenderPassSpecs.Descriptor.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		uiRenderPassSpecs.Descriptor.StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		uiRenderPassSpecs.Descriptor.StencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		uiRenderPassSpecs.Descriptor.StencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		uiRenderPassSpecs.Descriptor.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		uiRenderPassSpecs.Descriptor.FinalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		s_ImGuiPipeline.m_UIRenderPass = RenderPass::Create(uiRenderPassSpecs);
		s_ImGuiPipeline.m_UIFramebuffer = FrameBuffer::Create(geometryFrameBufferSpecs, s_ImGuiPipeline.m_UIRenderPass);

		Renderer::Dispatch(); //initialization

		vertexBuffer = new VulkanVertexBuffer(8);
		indexBuffer = new VulkanIndexBuffer(6);
		vertexBuffer->AddData(
			{ -0.5f, -0.5f,
			   0.5f, -0.5f,
			   0.5f, 0.5f,
			  -0.5f, 0.5f
			}
		);

		vertexBuffer->Load();

		indexBuffer->AddData(
			{ 0, 1, 2, 2, 3, 0 }
		);
		indexBuffer->Load();
	}

	void ViewportRenderer::Begin(Scene& scene) {
		const auto& meshView = scene.View<MeshComponent>();

		for (auto entity : meshView) {
			Entity e{ &scene, entity };
			MeshComponent& meshComponent = e.GetComponent<MeshComponent>();
			if (!meshComponent.IsValid()) continue;

			Renderer::SubmitMesh(s_GeometryPipeline, meshComponent.GetMesh(), e.GetComponent<TransformComponent>().GetMatrix());
		}

		s_ActiveScene = &scene;
	}

	void ViewportRenderer::Dispatch() {
		Renderer::BeginScene(*s_ActiveScene);
		Renderer::Dispatch(); //dispatch all the functions that should happen on the main render thread

		//do passes
		GeometryPass();
		UIPass();
		IDPass();

		PresentResult result = Renderer::RenderScene(); //render the actual scene (TODO: do multithreading)
		if (result == PresentResult::ERROR_OUT_OF_DATE_KHR || result == PresentResult::SUBOPTIMAL_KHR)
			OnWindowResize();
		Renderer::EndScene();
	}

	void ViewportRenderer::End() {
		Renderer::ClearDrawCommands();
	}

	void ViewportRenderer::OnWindowResize() {
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		swapChain.Recreate();
		Renderer::OnViewportResize();
		As(s_GeometryPipeline, VulkanPipeline)->Recreate();
		Renderer::Submit([]() {
			As(s_ImGuiPipeline.m_UIFramebuffer, VulkanFrameBuffer)->Recreate();
			s_ImGuiPipeline.m_UIRenderPass->Recreate();
		});
		//auto& extent = swapChain.GetExtent();
		//Renderer::SetViewportSize(extent.width, extent.height);
	}

	void ViewportRenderer::Destroy() {
		vertexBuffer->Destroy(); //TODO: Delete
		indexBuffer->Destroy(); //TODO: Delete

		delete vertexBuffer;
		delete indexBuffer;

		s_GeometryPipeline->Destroy();
		s_ImGuiPipeline.m_UIFramebuffer->Destroy();
		s_ImGuiPipeline.m_UIRenderPass->Destroy();
	}

	void ViewportRenderer::UIPass() {
		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		RenderCommand uiPipelineCommand = RenderCommand::BeginNew();
		uiPipelineCommand.Pipeline = nullptr; //imgui provides a pipeline
		uiPipelineCommand.Func = [this, swapChain](VkCommandBuffer commandBuffer) {
			const auto& targetFrameBuffer = As(s_ImGuiPipeline.m_UIFramebuffer, VulkanFrameBuffer)->GetVulkanHandles()[swapChain.GetCurrentImageIndex()];

			RenderPassBeginInfo beginInfo;
			beginInfo.CommandBuffer = commandBuffer;
			beginInfo.VulkanFrameBuffer = targetFrameBuffer;
			s_ImGuiPipeline.m_UIRenderPass->Begin(beginInfo);

			Renderer::s_UIPassFunc(commandBuffer);

			RenderPassEndInfo endInfo;
			endInfo.CommandBuffer = commandBuffer;
			endInfo.VulkanFrameBuffer = targetFrameBuffer;
			s_ImGuiPipeline.m_UIRenderPass->End(endInfo);
		};
		RenderCommand::End();
		Renderer::SubmitRenderCommand(uiPipelineCommand);
	}

	void ViewportRenderer::GeometryPass() {

		/*
		//TODO: Change this and add submeshes
		for (MeshDrawCommand& meshDrawCommand : m_MeshDrawCommands) {
			geometryPipelineCommand.Pipeline = As(meshDrawCommand.Pipeline, VulkanPipeline);
			geometryPipelineCommand.Func = [meshDrawCommand](VkCommandBuffer commandBuffer) {
				vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);
			};
		}
		*/

		//TODO: DELETE
		RenderCommand geometryPipelineCommand = RenderCommand::BeginNew();
		geometryPipelineCommand.Pipeline = As(ViewportRenderer::s_GeometryPipeline, VulkanPipeline);
		geometryPipelineCommand.Func = [this](VkCommandBuffer commandBuffer) {
			vertexBuffer->Bind({ commandBuffer });
			indexBuffer->Bind({ commandBuffer });

			vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);

			vertexBuffer->Unbind();
			indexBuffer->Unbind();
		};
		RenderCommand::End();

		auto& uniformBuffer = geometryPipelineCommand.Pipeline->GetUniformBuffers<VulkanUniformBuffer>(0);

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
		uniformBuffer->WriteToSets(VulkanSwapChain::GetCurrentFrameIndex());

		Renderer::SubmitRenderCommand(geometryPipelineCommand);
	}

	void ViewportRenderer::IDPass() {

	}
}
