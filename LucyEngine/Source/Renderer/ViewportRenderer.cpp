#include "lypch.h"
#include "ViewportRenderer.h"

#include "Renderer.h"

#include "Context/RHI.h"
#include "Context/VulkanPipeline.h"
#include "Context/VulkanSwapChain.h"
#include "Memory/Buffer/Vulkan/VulkanFrameBuffer.h"
#include "Memory/Buffer/Vulkan/VulkanUniformBuffer.h"

#include "Utils/Utils.h"

namespace Lucy {

	void ViewportRenderer::Init() {
		auto& pbrShader = Renderer::GetShaderLibrary().GetShader("LucyPBR");

		VulkanSwapChain& swapChain = VulkanSwapChain::Get();
		RenderArchitecture rhi = Renderer::GetCurrentRenderArchitecture();

		auto [width, height] = Utils::ReadAttributeFromIni("Viewport", "Size");
		Renderer::SetViewportSize(width, height);

		std::vector<ShaderLayoutElement> vertexLayout = {
				{ "a_Pos", ShaderDataSize::Float3 },
				{ "a_TextureCoords", ShaderDataSize::Float2 },
				{ "a_ID", ShaderDataSize::Float4 },
				{ "a_Normals", ShaderDataSize::Float3 },
				{ "a_Tangents", ShaderDataSize::Float3 },
				{ "a_BiTangents", ShaderDataSize::Float3 }
		};

		RenderPassCreateInfo geometryPassCreateInfo;
		geometryPassCreateInfo.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

		ImageCreateInfo geometryTextureCreateInfo;
		geometryTextureCreateInfo.Width = width;
		geometryTextureCreateInfo.Height = height;
		geometryTextureCreateInfo.Format = VK_FORMAT_R8G8B8A8_UNORM;
		geometryTextureCreateInfo.ImageType = ImageType::Type2D;
		geometryTextureCreateInfo.Target = ImageTarget::Color;
		geometryTextureCreateInfo.Parameter.Mag = VK_FILTER_LINEAR;
		geometryTextureCreateInfo.Parameter.Min = VK_FILTER_LINEAR;
		geometryTextureCreateInfo.Parameter.U = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		geometryTextureCreateInfo.Parameter.V = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		geometryTextureCreateInfo.Parameter.W = VK_SAMPLER_ADDRESS_MODE_REPEAT;

		FrameBufferCreateInfo geometryFrameBufferCreateInfo;
		geometryFrameBufferCreateInfo.Width = width;
		geometryFrameBufferCreateInfo.Height = height;

		PipelineCreateInfo geometryPipelineCreateInfo;
		geometryPipelineCreateInfo.VertexShaderLayout = VertexShaderLayout(vertexLayout);
		geometryPipelineCreateInfo.Topology = Topology::TRIANGLES;
		geometryPipelineCreateInfo.Rasterization = { true, VK_CULL_MODE_NONE, 1.0f, PolygonMode::FILL };
		geometryPipelineCreateInfo.Shader = pbrShader;

		if (rhi == RenderArchitecture::Vulkan) {
			Ref<VulkanRHIRenderPassDesc> vulkanRenderPassDesc = Memory::CreateRef<VulkanRHIRenderPassDesc>();
			vulkanRenderPassDesc->ColorAttachments.push_back(
				{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }
			);
			vulkanRenderPassDesc->DepthEnable = true; //enables the support for depth buffer
			vulkanRenderPassDesc->ColorDescriptor.Format = VK_FORMAT_R8G8B8A8_UNORM;
			vulkanRenderPassDesc->ColorDescriptor.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			vulkanRenderPassDesc->ColorDescriptor.StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
			vulkanRenderPassDesc->ColorDescriptor.StencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			vulkanRenderPassDesc->ColorDescriptor.StencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			vulkanRenderPassDesc->ColorDescriptor.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			vulkanRenderPassDesc->ColorDescriptor.FinalLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR;
			geometryPassCreateInfo.InternalInfo = vulkanRenderPassDesc;

			Ref<VulkanRHIImageDesc> vulkanTextureDesc = Memory::CreateRef<VulkanRHIImageDesc>();
			vulkanTextureDesc->GenerateSampler = true;
			vulkanTextureDesc->ImGuiUsage = true;

			geometryTextureCreateInfo.InternalInfo = vulkanTextureDesc;

			Ref<VulkanRHIFrameBufferDesc> vulkanFrameBufferDesc = Memory::CreateRef<VulkanRHIFrameBufferDesc>();
			vulkanFrameBufferDesc->ImageBuffers.reserve(swapChain.GetMaxFramesInFlight());
			for (uint32_t i = 0; i < swapChain.GetMaxFramesInFlight(); i++)
				vulkanFrameBufferDesc->ImageBuffers.emplace_back(Image2D::Create(geometryTextureCreateInfo).As<VulkanImage2D>());
			vulkanFrameBufferDesc->RenderPass = RenderPass::Create(geometryPassCreateInfo);

			geometryFrameBufferCreateInfo.InternalInfo = vulkanFrameBufferDesc;
			geometryPipelineCreateInfo.RenderPass = vulkanFrameBufferDesc->RenderPass;
		}

		geometryPipelineCreateInfo.FrameBuffer = FrameBuffer::Create(geometryFrameBufferCreateInfo);
		s_GeometryPipeline = Pipeline::Create(geometryPipelineCreateInfo);

		/*
		----ImGui (for Vulkan; does not need a separate pipeline)----
		*/

		if (rhi == RenderArchitecture::Vulkan) {
			RenderPassCreateInfo uiRenderPassCreateInfo;
			uiRenderPassCreateInfo.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

			Ref<VulkanRHIRenderPassDesc> vulkanRenderPassDesc = Memory::CreateRef<VulkanRHIRenderPassDesc>();
			vulkanRenderPassDesc->ColorAttachments.push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
			vulkanRenderPassDesc->ColorDescriptor.Format = swapChain.GetSurfaceFormat().format;
			vulkanRenderPassDesc->ColorDescriptor.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			vulkanRenderPassDesc->ColorDescriptor.StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
			vulkanRenderPassDesc->ColorDescriptor.StencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			vulkanRenderPassDesc->ColorDescriptor.StencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			vulkanRenderPassDesc->ColorDescriptor.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			vulkanRenderPassDesc->ColorDescriptor.FinalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			uiRenderPassCreateInfo.InternalInfo = vulkanRenderPassDesc;
			s_ImGuiPipeline.UIRenderPass = RenderPass::Create(uiRenderPassCreateInfo);

			FrameBufferCreateInfo uiFrameBufferCreateInfo;
			uiFrameBufferCreateInfo.Width = swapChain.GetExtent().width;
			uiFrameBufferCreateInfo.Height = swapChain.GetExtent().height;

			Ref<VulkanRHIFrameBufferDesc> vulkanFrameBufferDesc = Memory::CreateRef<VulkanRHIFrameBufferDesc>();
			vulkanFrameBufferDesc->ImageViews = swapChain.GetImageViews();
			vulkanFrameBufferDesc->RenderPass = s_ImGuiPipeline.UIRenderPass;

			uiFrameBufferCreateInfo.InternalInfo = vulkanFrameBufferDesc;
			s_ImGuiPipeline.UIFramebuffer = FrameBuffer::Create(uiFrameBufferCreateInfo);
		}
	}

	void ViewportRenderer::Begin(Scene& scene) {
		Renderer::Dispatch(); //dispatch all the functions that should happen on the main render thread (before the passes)
		Renderer::BeginScene(scene);
	}

	void ViewportRenderer::Dispatch(Scene& scene) {
		auto& cameraBuffer = s_GeometryPipeline->GetUniformBuffers<VulkanUniformBuffer>("Camera");

		EditorCamera& camera = scene.GetEditorCamera();
		auto vp = camera.GetVP();
		cameraBuffer->SetData((uint8_t*)&vp, sizeof(vp));

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
		s_GeometryPipeline->Destroy();
		s_ImGuiPipeline.UIFramebuffer->Destroy();
		s_ImGuiPipeline.UIRenderPass->Destroy();
	}

	void ViewportRenderer::GeometryPass() {
		Renderer::RecordStaticMeshToCommandQueue(s_GeometryPipeline, [](Ref<MeshDrawCommand> staticMeshDrawCommand) {
			const Ref<Mesh>& staticMesh = staticMeshDrawCommand->Mesh;
			const glm::mat4& entityTransform = staticMeshDrawCommand->EntityTransform;

			PushConstant& meshPushConstant = s_GeometryPipeline->GetPushConstants("LocalPushConstant");
			
			const std::vector<Ref<Material>>& materials = staticMesh->GetMaterials();
			std::vector<Submesh>& submeshes = staticMesh->GetSubmeshes();

			Renderer::BindBuffers(staticMesh);

			for (uint32_t i = 0; i < submeshes.size(); i++) {
				Submesh& submesh = submeshes[i];
				const Ref<Material>& material = materials[submesh.MaterialIndex];

				PushConstantData pushConstantData;
				pushConstantData.FinalTransform = entityTransform * submesh.Transform;
				pushConstantData.MaterialID = material->GetID();
				meshPushConstant.SetData((uint8_t*)&pushConstantData, sizeof(pushConstantData));

				Renderer::BindPushConstant(s_GeometryPipeline, meshPushConstant);
				Renderer::DrawIndexed(submesh.IndexCount, 1, submesh.BaseIndexCount, submesh.BaseVertexCount, 0);
			}
		});
	}

	void ViewportRenderer::IDPass() {
		//TODO: later
	}

	void ViewportRenderer::UIPass() {
		Renderer::UIPass(s_ImGuiPipeline);
	}
}
