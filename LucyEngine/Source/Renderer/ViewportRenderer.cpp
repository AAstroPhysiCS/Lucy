#include "lypch.h"
#include "ViewportRenderer.h"

#include "Renderer.h"

#include "RenderDevice.h"
#include "Context/VulkanPipeline.h"
#include "Context/VulkanSwapChain.h"
#include "Memory/Buffer/Vulkan/VulkanFrameBuffer.h"
#include "Memory/Buffer/Vulkan/VulkanUniformBuffer.h"

#include "Utils/Utils.h"

namespace Lucy {

	void ViewportRenderer::Init(RenderArchitecture arch, Ref<Window> window) {
		Renderer::Init(arch, window);

		auto& pbrShader = Renderer::GetShaderLibrary().GetShader("LucyPBR");
		auto& idShader = Renderer::GetShaderLibrary().GetShader("LucyID");

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

#pragma region GeometryPipeline

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
		geometryPipelineCreateInfo.Rasterization = { true, CullingMode::None, 1.0f, PolygonMode::FILL };
		geometryPipelineCreateInfo.Shader = pbrShader;

		if (arch == RenderArchitecture::Vulkan) {
			VulkanSwapChain& swapChain = VulkanSwapChain::Get();

			Ref<VulkanRenderPassInfo> vulkanRenderPassInfo = Memory::CreateRef<VulkanRenderPassInfo>();
			vulkanRenderPassInfo->ColorAttachments.push_back(
				{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }
			);
			vulkanRenderPassInfo->DepthEnable = true; //enables the support for depth buffer
			vulkanRenderPassInfo->ColorDescriptor.Format = (VkFormat) geometryTextureCreateInfo.Format;
			vulkanRenderPassInfo->ColorDescriptor.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			vulkanRenderPassInfo->ColorDescriptor.StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
			vulkanRenderPassInfo->ColorDescriptor.StencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			vulkanRenderPassInfo->ColorDescriptor.StencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			vulkanRenderPassInfo->ColorDescriptor.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			vulkanRenderPassInfo->ColorDescriptor.FinalLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR;
			geometryPassCreateInfo.InternalInfo = vulkanRenderPassInfo;

			Ref<VulkanImageInfo> vulkanTextureInfo = Memory::CreateRef<VulkanImageInfo>();
			vulkanTextureInfo->GenerateSampler = true;
			vulkanTextureInfo->ImGuiUsage = true;

			geometryTextureCreateInfo.InternalInfo = vulkanTextureInfo;

			Ref<VulkanFrameBufferInfo> vulkanFrameBufferInfo = Memory::CreateRef<VulkanFrameBufferInfo>();
			vulkanFrameBufferInfo->ImageBuffers.reserve(swapChain.GetMaxFramesInFlight());
			for (uint32_t i = 0; i < swapChain.GetMaxFramesInFlight(); i++)
				vulkanFrameBufferInfo->ImageBuffers.emplace_back(Image2D::Create(geometryTextureCreateInfo).As<VulkanImage2D>());
			vulkanFrameBufferInfo->RenderPass = RenderPass::Create(geometryPassCreateInfo);

			geometryFrameBufferCreateInfo.InternalInfo = vulkanFrameBufferInfo;
			geometryPipelineCreateInfo.RenderPass = vulkanFrameBufferInfo->RenderPass;
		}
		geometryPipelineCreateInfo.FrameBuffer = FrameBuffer::Create(geometryFrameBufferCreateInfo);
		s_GeometryPipeline = Pipeline::Create(geometryPipelineCreateInfo);

#pragma region IDPipeline

		RenderPassCreateInfo idPassCreateInfo = geometryPassCreateInfo;

		ImageCreateInfo idTextureCreateInfo;
		idTextureCreateInfo = geometryTextureCreateInfo;
		idTextureCreateInfo.Format = VK_FORMAT_R8G8B8A8_UNORM;

		FrameBufferCreateInfo idFrameBufferCreateInfo = geometryFrameBufferCreateInfo;

		PipelineCreateInfo idPipelineCreateInfo = geometryPipelineCreateInfo;
		idPipelineCreateInfo.Shader = idShader;

		if (arch == RenderArchitecture::Vulkan) {
			VulkanSwapChain& swapChain = VulkanSwapChain::Get();
			
			Ref<VulkanRenderPassInfo> idVulkanRenderPassInfo = Memory::CreateRef<VulkanRenderPassInfo>();
			idVulkanRenderPassInfo->ColorAttachments.push_back(
				{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }
			);
			idVulkanRenderPassInfo->DepthEnable = true; //enables the support for depth buffer
			idVulkanRenderPassInfo->ColorDescriptor.Format = (VkFormat) idTextureCreateInfo.Format;
			idVulkanRenderPassInfo->ColorDescriptor.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			idVulkanRenderPassInfo->ColorDescriptor.StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
			idVulkanRenderPassInfo->ColorDescriptor.StencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			idVulkanRenderPassInfo->ColorDescriptor.StencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			idVulkanRenderPassInfo->ColorDescriptor.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			idVulkanRenderPassInfo->ColorDescriptor.FinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			idPassCreateInfo.InternalInfo = idVulkanRenderPassInfo;

			Ref<VulkanImageInfo> idVulkanTextureInfo = Memory::CreateRef<VulkanImageInfo>();
			idVulkanTextureInfo->GenerateSampler = true;

			idTextureCreateInfo.InternalInfo = idVulkanTextureInfo;

			Ref<VulkanFrameBufferInfo> idVulkanFrameBufferInfo = Memory::CreateRef<VulkanFrameBufferInfo>();
			idVulkanFrameBufferInfo->ImageBuffers.reserve(swapChain.GetMaxFramesInFlight());
			for (uint32_t i = 0; i < swapChain.GetMaxFramesInFlight(); i++)
				idVulkanFrameBufferInfo->ImageBuffers.emplace_back(Image2D::Create(idTextureCreateInfo).As<VulkanImage2D>());
			idVulkanFrameBufferInfo->RenderPass = RenderPass::Create(idPassCreateInfo);

			idFrameBufferCreateInfo.InternalInfo = idVulkanFrameBufferInfo;
			idPipelineCreateInfo.RenderPass = idVulkanFrameBufferInfo->RenderPass;
		}
		idPipelineCreateInfo.FrameBuffer = FrameBuffer::Create(idFrameBufferCreateInfo);
		s_IDPipeline = Pipeline::Create(idPipelineCreateInfo);

#pragma region ImGuiPipeline
		/*
		----ImGui (for Vulkan; does not need a separate pipeline)----
		*/
		if (arch == RenderArchitecture::Vulkan) {
			VulkanSwapChain& swapChain = VulkanSwapChain::Get();

			RenderPassCreateInfo uiRenderPassCreateInfo;
			uiRenderPassCreateInfo.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

			Ref<VulkanRenderPassInfo> vulkanRenderPassInfo = Memory::CreateRef<VulkanRenderPassInfo>();
			vulkanRenderPassInfo->ColorAttachments.push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
			vulkanRenderPassInfo->ColorDescriptor.Format = swapChain.GetSurfaceFormat().format;
			vulkanRenderPassInfo->ColorDescriptor.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			vulkanRenderPassInfo->ColorDescriptor.StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
			vulkanRenderPassInfo->ColorDescriptor.StencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			vulkanRenderPassInfo->ColorDescriptor.StencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			vulkanRenderPassInfo->ColorDescriptor.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			vulkanRenderPassInfo->ColorDescriptor.FinalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			uiRenderPassCreateInfo.InternalInfo = vulkanRenderPassInfo;
			s_ImGuiPipeline.UIRenderPass = RenderPass::Create(uiRenderPassCreateInfo);

			FrameBufferCreateInfo uiFrameBufferCreateInfo;
			uiFrameBufferCreateInfo.Width = swapChain.GetExtent().width;
			uiFrameBufferCreateInfo.Height = swapChain.GetExtent().height;

			Ref<VulkanFrameBufferInfo> vulkanFrameBufferInfo = Memory::CreateRef<VulkanFrameBufferInfo>();
			vulkanFrameBufferInfo->ImageViews = swapChain.GetImageViews();
			vulkanFrameBufferInfo->RenderPass = s_ImGuiPipeline.UIRenderPass;

			uiFrameBufferCreateInfo.InternalInfo = vulkanFrameBufferInfo;
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

		//IDPass();
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
		s_IDPipeline->Destroy();

		s_ImGuiPipeline.UIFramebuffer->Destroy();
		s_ImGuiPipeline.UIRenderPass->Destroy();

		Renderer::Destroy();
	}

	void ViewportRenderer::WaitForDevice() {
		Renderer::WaitForDevice();
	}

	void ViewportRenderer::GeometryPass() {
		Renderer::RecordStaticMeshToCommandQueue(s_GeometryPipeline, [](VkCommandBuffer commandBuffer, Ref<MeshDrawCommand> staticMeshDrawCommand) {
			const Ref<Mesh>& staticMesh = staticMeshDrawCommand->Mesh;
			const glm::mat4& entityTransform = staticMeshDrawCommand->EntityTransform;

			PushConstant& meshPushConstant = s_GeometryPipeline->GetPushConstants("LocalPushConstant");
			
			const std::vector<Ref<Material>>& materials = staticMesh->GetMaterials();
			std::vector<Submesh>& submeshes = staticMesh->GetSubmeshes();

			Renderer::BindBuffers(commandBuffer, staticMesh);

			for (uint32_t i = 0; i < submeshes.size(); i++) {
				Submesh& submesh = submeshes[i];
				const Ref<Material>& material = materials[submesh.MaterialIndex];

				PushConstantData pushConstantData;
				pushConstantData.FinalTransform = entityTransform * submesh.Transform;
				pushConstantData.MaterialID = material->GetID();
				meshPushConstant.SetData((uint8_t*)&pushConstantData, sizeof(pushConstantData));

				Renderer::BindPushConstant(commandBuffer, s_GeometryPipeline, meshPushConstant);
				Renderer::DrawIndexed(commandBuffer, submesh.IndexCount, 1, submesh.BaseIndexCount, submesh.BaseVertexCount, 0);
			}
		});
	}

	void ViewportRenderer::IDPass() {
		Renderer::RecordStaticMeshToCommandQueue(s_IDPipeline, [](VkCommandBuffer commandBuffer, Ref<MeshDrawCommand> staticMeshDrawCommand) {
			const Ref<Mesh>& staticMesh = staticMeshDrawCommand->Mesh;
			const glm::mat4& entityTransform = staticMeshDrawCommand->EntityTransform;

			PushConstant& meshPushConstant = s_IDPipeline->GetPushConstants("LocalPushConstant");

			const std::vector<Ref<Material>>& materials = staticMesh->GetMaterials();
			std::vector<Submesh>& submeshes = staticMesh->GetSubmeshes();

			Renderer::BindBuffers(commandBuffer, staticMesh);

			for (uint32_t i = 0; i < submeshes.size(); i++) {
				Submesh& submesh = submeshes[i];
				const Ref<Material>& material = materials[submesh.MaterialIndex];

				PushConstantData pushConstantData;
				pushConstantData.FinalTransform = entityTransform * submesh.Transform;
				meshPushConstant.SetData((uint8_t*)&pushConstantData, sizeof(pushConstantData.FinalTransform));

				Renderer::BindPushConstant(commandBuffer, s_IDPipeline, meshPushConstant);
				Renderer::DrawIndexed(commandBuffer, submesh.IndexCount, 1, submesh.BaseIndexCount, submesh.BaseVertexCount, 0);
			}
		});
	}

	void ViewportRenderer::UIPass() {
		Renderer::UIPass(s_ImGuiPipeline);
	}
}
