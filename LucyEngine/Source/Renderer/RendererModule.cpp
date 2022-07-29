#include "lypch.h"
#include "RendererModule.h"
#include "Renderer/Renderer.h"

#include "Events/EventDispatcher.h"
#include "Events/WindowEvent.h"

#include "Context/VulkanSwapChain.h"

#include "Memory/Buffer/Vulkan/VulkanFrameBuffer.h"
#include "Memory/Buffer/Vulkan/VulkanUniformBuffer.h"

#include "Scene/Entity.h"

namespace Lucy {

	/* --- Individual Passes --- (TODO: abstract this maybe?) */
	namespace ViewportRenderer {
		void GeometryPass(void* commandBuffer, Ref<Pipeline> geometryPipeline, RenderCommand* staticMeshRenderCommand);
		void IDPass(void* commandBuffer, Ref<Pipeline> geometryPipeline, RenderCommand* staticMeshRenderCommand);
	}

	/* --- Individual Resource Handles --- (TODO: abstract this maybe?) */
	static RenderCommandResourceHandle g_GeometryPassHandle;
	static RenderCommandResourceHandle g_IDPassHandle;

	RendererModule::RendererModule(RenderArchitecture arch, Ref<Window> window, Ref<Scene> scene)
		: Module(window, scene) {
		Renderer::Init(arch, window);

		window->SetTitle(fmt::format("{0} - Windows x64 {1}", m_Window->GetTitle(),
						 arch == RenderArchitecture::Vulkan ? "Vulkan" : "DirectX12").c_str());

		auto& [viewportWidth, viewportHeight] = Renderer::GetViewportArea();

		ShaderLibrary& shaderLibrary = ShaderLibrary::Get();
		auto& pbrShader = shaderLibrary.GetShader("LucyPBR");
		auto& idShader = shaderLibrary.GetShader("LucyID");

		std::vector<ShaderLayoutElement> vertexLayout = {
				{ "a_Pos", ShaderDataSize::Float3 },
				{ "a_TextureCoords", ShaderDataSize::Float2 },
				{ "a_ID", ShaderDataSize::Float3 },
				{ "a_Normals", ShaderDataSize::Float3 },
				{ "a_Tangents", ShaderDataSize::Float3 },
				{ "a_BiTangents", ShaderDataSize::Float3 }
		};

		uint32_t maxFramesInFlight = 1;
		if (arch == RenderArchitecture::Vulkan)
			maxFramesInFlight = VulkanSwapChain::Get().GetMaxFramesInFlight();

#pragma region GeometryPipeline

		RenderPassCreateInfo geometryPassCreateInfo;
		geometryPassCreateInfo.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		geometryPassCreateInfo.DepthEnable = true; //enables the support for depth buffer

		ImageCreateInfo geometryTextureCreateInfo;
		geometryTextureCreateInfo.Width = viewportWidth;
		geometryTextureCreateInfo.Height = viewportHeight;
		geometryTextureCreateInfo.Format = VK_FORMAT_R8G8B8A8_UNORM;
		geometryTextureCreateInfo.ImageType = ImageType::Type2D;
		geometryTextureCreateInfo.Target = ImageTarget::Color;
		geometryTextureCreateInfo.Parameter.Mag = VK_FILTER_LINEAR;
		geometryTextureCreateInfo.Parameter.Min = VK_FILTER_LINEAR;
		geometryTextureCreateInfo.Parameter.U = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		geometryTextureCreateInfo.Parameter.V = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		geometryTextureCreateInfo.Parameter.W = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		geometryTextureCreateInfo.GenerateSampler = true;
		geometryTextureCreateInfo.ImGuiUsage = true;

		FrameBufferCreateInfo geometryFrameBufferCreateInfo;
		geometryFrameBufferCreateInfo.Width = viewportWidth;
		geometryFrameBufferCreateInfo.Height = viewportHeight;

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
			vulkanRenderPassInfo->ColorDescriptor.Format = (VkFormat)geometryTextureCreateInfo.Format;
			vulkanRenderPassInfo->ColorDescriptor.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			vulkanRenderPassInfo->ColorDescriptor.StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
			vulkanRenderPassInfo->ColorDescriptor.StencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			vulkanRenderPassInfo->ColorDescriptor.StencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			vulkanRenderPassInfo->ColorDescriptor.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			vulkanRenderPassInfo->ColorDescriptor.FinalLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR;
			geometryPassCreateInfo.InternalInfo = vulkanRenderPassInfo;

			Ref<VulkanFrameBufferInfo> vulkanFrameBufferInfo = Memory::CreateRef<VulkanFrameBufferInfo>();
			vulkanFrameBufferInfo->RenderPass = RenderPass::Create(geometryPassCreateInfo);

			geometryFrameBufferCreateInfo.InternalInfo = vulkanFrameBufferInfo;
			geometryPipelineCreateInfo.RenderPass = vulkanFrameBufferInfo->RenderPass;
		}

		geometryFrameBufferCreateInfo.ImageBuffers.reserve(maxFramesInFlight);
		for (uint32_t i = 0; i < maxFramesInFlight; i++)
			geometryFrameBufferCreateInfo.ImageBuffers.emplace_back(Image2D::Create(geometryTextureCreateInfo));

		geometryPipelineCreateInfo.FrameBuffer = FrameBuffer::Create(geometryFrameBufferCreateInfo);
		m_GeometryPipeline = Pipeline::Create(geometryPipelineCreateInfo);
#pragma endregion GeometryPipeline

#pragma region IDPipeline

		RenderPassCreateInfo idPassCreateInfo = geometryPassCreateInfo;
		ImageCreateInfo idTextureCreateInfo = geometryTextureCreateInfo;
		idTextureCreateInfo.AdditionalUsageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		idTextureCreateInfo.ImGuiUsage = false;

		FrameBufferCreateInfo idFrameBufferCreateInfo = geometryFrameBufferCreateInfo;
		idFrameBufferCreateInfo.ImageBuffers.clear();

		PipelineCreateInfo idPipelineCreateInfo = geometryPipelineCreateInfo;
		idPipelineCreateInfo.Shader = idShader;

		if (arch == RenderArchitecture::Vulkan) {
			VulkanSwapChain& swapChain = VulkanSwapChain::Get();

			Ref<VulkanRenderPassInfo> idVulkanRenderPassInfo = Memory::CreateRef<VulkanRenderPassInfo>();
			idVulkanRenderPassInfo->ColorAttachments.push_back(
				{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }
			);
			idVulkanRenderPassInfo->ColorDescriptor.Format = (VkFormat)idTextureCreateInfo.Format;
			idVulkanRenderPassInfo->ColorDescriptor.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			idVulkanRenderPassInfo->ColorDescriptor.StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
			idVulkanRenderPassInfo->ColorDescriptor.StencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			idVulkanRenderPassInfo->ColorDescriptor.StencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			idVulkanRenderPassInfo->ColorDescriptor.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			idVulkanRenderPassInfo->ColorDescriptor.FinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			idPassCreateInfo.InternalInfo = idVulkanRenderPassInfo;

			Ref<VulkanFrameBufferInfo> idVulkanFrameBufferInfo = Memory::CreateRef<VulkanFrameBufferInfo>();
			idVulkanFrameBufferInfo->RenderPass = RenderPass::Create(idPassCreateInfo);

			idFrameBufferCreateInfo.InternalInfo = idVulkanFrameBufferInfo;
			idPipelineCreateInfo.RenderPass = idVulkanFrameBufferInfo->RenderPass;
		}

		idFrameBufferCreateInfo.ImageBuffers.reserve(maxFramesInFlight);
		for (uint32_t i = 0; i < maxFramesInFlight; i++)
			idFrameBufferCreateInfo.ImageBuffers.emplace_back(Image2D::Create(idTextureCreateInfo));

		idPipelineCreateInfo.FrameBuffer = FrameBuffer::Create(idFrameBufferCreateInfo);
		m_IDPipeline = Pipeline::Create(idPipelineCreateInfo);
#pragma endregion IDPipeline

		g_GeometryPassHandle = Renderer::CreateRenderPassResource(ViewportRenderer::GeometryPass, m_GeometryPipeline);
		g_IDPassHandle = Renderer::CreateRenderPassResource(ViewportRenderer::IDPass, m_IDPipeline);
	}

	void RendererModule::Begin() {
		Renderer::BeginScene(m_Scene);

		const auto& meshView = m_Scene->View<MeshComponent>();

		for (auto entity : meshView) {
			Entity e{ m_Scene.Get(), entity };
			MeshComponent meshComponent = e.GetComponent<MeshComponent>();
			if (!meshComponent.IsValid())
				continue;

			Ref<Mesh>& mesh = meshComponent.GetMesh();

			auto& materials = mesh->GetMaterials();
			for (Submesh& submesh : mesh->GetSubmeshes()) {
				const Ref<Material>& material = materials[submesh.MaterialIndex];
				material->Update(m_GeometryPipeline);
			}

			//High priority stuff must be called earlier than low, since those meshes get render first.
			//I won't be sorting the render commands accordingly, the sort must happen here (for performance reason)

			Renderer::EnqueueRenderCommand<StaticMeshRenderCommand>(g_GeometryPassHandle, Priority::LOW, meshComponent.GetMesh(), e.GetComponent<TransformComponent>().GetMatrix());
			Renderer::EnqueueRenderCommand<StaticMeshRenderCommand>(g_IDPassHandle, Priority::LOW, meshComponent.GetMesh(), e.GetComponent<TransformComponent>().GetMatrix());
		}
	}

	void RendererModule::OnRender() {
		EditorCamera& camera = m_Scene->GetEditorCamera();
		auto vp = camera.GetVP();

		auto& cameraBuffer = m_GeometryPipeline->GetUniformBuffers<VulkanUniformBuffer>("Camera");
		cameraBuffer->SetData((uint8_t*)&vp, sizeof(vp));

		auto& cameraBufferID = m_IDPipeline->GetUniformBuffers<VulkanUniformBuffer>("Camera");
		cameraBufferID->SetData((uint8_t*)&vp, sizeof(vp));

		//TODO: Refactor descriptor set api, make it decentralized, meaning make it pipeline independent
		//example: DescriptorSetCache::UpdateAllDescriptorSets();
		Renderer::UpdateDescriptorSets(m_GeometryPipeline);
		Renderer::UpdateDescriptorSets(m_IDPipeline);

		Renderer::RenderScene();
	}

	void RendererModule::End() {
		PresentResult result = Renderer::EndScene();
		if (result == PresentResult::ERROR_OUT_OF_DATE_KHR || result == PresentResult::SUBOPTIMAL_KHR)
			OnWindowResize();
	}

	void RendererModule::OnEvent(Event& e) {
		EventDispatcher& dispatcher = EventDispatcher::GetInstance();
		dispatcher.Dispatch<WindowResizeEvent>(e, EventType::WindowResizeEvent, [&](const WindowResizeEvent& e) {
			OnWindowResize();
		});
	}

	void RendererModule::Destroy() {
		ShaderLibrary::Get().Destroy();

		m_GeometryPipeline->Destroy();
		m_IDPipeline->Destroy();

		Renderer::Destroy();
	}

	void RendererModule::Wait() {
		Renderer::WaitForDevice();
	}

	void RendererModule::OnWindowResize() {
		Renderer::OnWindowResize();

		auto& [viewportWidth, viewportHeight] = Renderer::GetViewportArea();
		m_GeometryPipeline->Recreate(viewportWidth, viewportHeight);
		m_IDPipeline->Recreate(viewportWidth, viewportHeight);
	}

	namespace ViewportRenderer {

		void RenderAllMeshes(VkCommandBuffer commandBuffer, const Ref<Pipeline>& pipeline, StaticMeshRenderCommand* staticMeshRenderCommand) {
			const Ref<Mesh>& staticMesh = staticMeshRenderCommand->Mesh;
			const glm::mat4& entityTransform = staticMeshRenderCommand->EntityTransform;

			const std::vector<Ref<Material>>& materials = staticMesh->GetMaterials();
			std::vector<Submesh>& submeshes = staticMesh->GetSubmeshes();

			Renderer::BindPipeline(commandBuffer, pipeline);
			Renderer::BindAllDescriptorSets(commandBuffer, pipeline);
			Renderer::BindBuffers(commandBuffer, staticMesh);

			PushConstant& meshPushConstant = pipeline->GetPushConstants("LocalPushConstant");

			for (uint32_t i = 0; i < submeshes.size(); i++) {
				Submesh& submesh = submeshes[i];
				const Ref<Material>& material = materials[submesh.MaterialIndex];

				PushConstantData pushConstantData;
				pushConstantData.FinalTransform = entityTransform * submesh.Transform;
				pushConstantData.MaterialID = material->GetID();
				meshPushConstant.SetData((uint8_t*)&pushConstantData, sizeof(pushConstantData));

				Renderer::BindPushConstant(commandBuffer, pipeline, meshPushConstant);
				Renderer::DrawIndexed(commandBuffer, submesh.IndexCount, 1, submesh.BaseIndexCount, submesh.BaseVertexCount, 0);
			}
		}

		void GeometryPass(void* commandBuffer, Ref<Pipeline> geometryPipeline, RenderCommand* staticMeshRenderCommand) {
			RenderAllMeshes((VkCommandBuffer)commandBuffer, geometryPipeline, (StaticMeshRenderCommand*)staticMeshRenderCommand);
		}

		void IDPass(void* commandBuffer, Ref<Pipeline> idPipeline, RenderCommand* staticMeshRenderCommand) {
			RenderAllMeshes((VkCommandBuffer)commandBuffer, idPipeline, (StaticMeshRenderCommand*)staticMeshRenderCommand);
		}
	}
}
