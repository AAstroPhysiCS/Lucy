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

	/* --- Individual Resource Handles --- */
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

		const uint32_t maxFramesInFlight = Renderer::GetMaxFramesInFlight();

#pragma region GeometryPipeline

		RenderPassCreateInfo geometryPassCreateInfo;
		geometryPassCreateInfo.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		geometryPassCreateInfo.DepthEnable = true; //enables the support for depth buffer

		ImageCreateInfo geometryTextureCreateInfo;
		geometryTextureCreateInfo.Width = viewportWidth;
		geometryTextureCreateInfo.Height = viewportHeight;
		geometryTextureCreateInfo.Format = ImageFormat::R8G8B8A8_UNORM;
		geometryTextureCreateInfo.ImageType = ImageType::Type2D;
		geometryTextureCreateInfo.Target = ImageTarget::Color;
		geometryTextureCreateInfo.Parameter.Mag = ImageFilterMode::LINEAR;
		geometryTextureCreateInfo.Parameter.Min = ImageFilterMode::LINEAR;
		geometryTextureCreateInfo.Parameter.U = ImageAddressMode::REPEAT;
		geometryTextureCreateInfo.Parameter.V = ImageAddressMode::REPEAT;
		geometryTextureCreateInfo.Parameter.W = ImageAddressMode::REPEAT;
		geometryTextureCreateInfo.GenerateSampler = true;
		geometryTextureCreateInfo.ImGuiUsage = true;

		FrameBufferCreateInfo geometryFrameBufferCreateInfo;
		geometryFrameBufferCreateInfo.Width = viewportWidth;
		geometryFrameBufferCreateInfo.Height = viewportHeight;

		geometryFrameBufferCreateInfo.ImageBuffers.reserve(maxFramesInFlight);
		for (uint32_t i = 0; i < maxFramesInFlight; i++)
			geometryFrameBufferCreateInfo.ImageBuffers.emplace_back(Image2D::Create(geometryTextureCreateInfo));

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
			vulkanRenderPassInfo->ColorDescriptor.Format = geometryTextureCreateInfo.Format;
			vulkanRenderPassInfo->ColorDescriptor.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			vulkanRenderPassInfo->ColorDescriptor.StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
			vulkanRenderPassInfo->ColorDescriptor.StencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			vulkanRenderPassInfo->ColorDescriptor.StencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			vulkanRenderPassInfo->ColorDescriptor.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			vulkanRenderPassInfo->ColorDescriptor.FinalLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR;

			geometryPassCreateInfo.InternalInfo = vulkanRenderPassInfo;
		}

		Ref<RenderPass> geometryRenderPass = RenderPass::Create(geometryPassCreateInfo);
		geometryPipelineCreateInfo.RenderPass = geometryRenderPass;
		geometryFrameBufferCreateInfo.RenderPass = geometryRenderPass;

		Ref<FrameBuffer> geometryFrameBuffer = FrameBuffer::Create(geometryFrameBufferCreateInfo);
		geometryPipelineCreateInfo.FrameBuffer = geometryFrameBuffer;

		m_GeometryPipeline = Pipeline::Create(geometryPipelineCreateInfo);
#pragma endregion GeometryPipeline

#pragma region IDPipeline

		RenderPassCreateInfo idPassCreateInfo = geometryPassCreateInfo;
		ImageCreateInfo idTextureCreateInfo = geometryTextureCreateInfo;
		idTextureCreateInfo.AdditionalUsageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		idTextureCreateInfo.ImGuiUsage = false;

		FrameBufferCreateInfo idFrameBufferCreateInfo = geometryFrameBufferCreateInfo;
		idFrameBufferCreateInfo.ImageBuffers.clear();
		idFrameBufferCreateInfo.ImageBuffers.reserve(maxFramesInFlight);
		for (uint32_t i = 0; i < maxFramesInFlight; i++)
			idFrameBufferCreateInfo.ImageBuffers.emplace_back(Image2D::Create(idTextureCreateInfo));

		PipelineCreateInfo idPipelineCreateInfo = geometryPipelineCreateInfo;
		idPipelineCreateInfo.Shader = idShader;

		if (arch == RenderArchitecture::Vulkan) {
			VulkanSwapChain& swapChain = VulkanSwapChain::Get();

			Ref<VulkanRenderPassInfo> idVulkanRenderPassInfo = Memory::CreateRef<VulkanRenderPassInfo>();
			idVulkanRenderPassInfo->ColorAttachments.push_back(
				{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }
			);
			idVulkanRenderPassInfo->ColorDescriptor.Format = idTextureCreateInfo.Format;
			idVulkanRenderPassInfo->ColorDescriptor.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			idVulkanRenderPassInfo->ColorDescriptor.StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
			idVulkanRenderPassInfo->ColorDescriptor.StencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			idVulkanRenderPassInfo->ColorDescriptor.StencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			idVulkanRenderPassInfo->ColorDescriptor.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			idVulkanRenderPassInfo->ColorDescriptor.FinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			idPassCreateInfo.InternalInfo = idVulkanRenderPassInfo;
		}

		Ref<RenderPass> idRenderPass = RenderPass::Create(idPassCreateInfo);
		idPipelineCreateInfo.RenderPass = idRenderPass;
		idFrameBufferCreateInfo.RenderPass = idRenderPass;

		Ref<FrameBuffer> idFrameBuffer = FrameBuffer::Create(idFrameBufferCreateInfo);
		idPipelineCreateInfo.FrameBuffer = idFrameBuffer;

		m_IDPipeline = Pipeline::Create(idPipelineCreateInfo);
#pragma endregion IDPipeline

		g_GeometryPassHandle = Renderer::CreateRenderPassResource(GeometryPass, m_GeometryPipeline);
		g_IDPassHandle = Renderer::CreateRenderPassResource(IDPass, m_IDPipeline);
	}

	void RendererModule::Begin() {
		LUCY_PROFILE_NEW_EVENT("RendererModule::Begin");

		Renderer::BeginScene(m_Scene);

		auto& directionalLightView = m_Scene->View<DirectionalLightComponent>();
		for (auto& entity : directionalLightView) {
			Entity e{ m_Scene.Get(), entity };
			DirectionalLightComponent& lightComponent = e.GetComponent<DirectionalLightComponent>();

			auto& lightningAttributes = m_GeometryPipeline->GetUniformBuffers<VulkanUniformBuffer>("LucyLightningValues");
			lightningAttributes->SetData((uint8_t*)&lightComponent, sizeof(DirectionalLightComponent));
		}

		auto& meshView = m_Scene->View<MeshComponent>();
		for (entt::sparse_set::reverse_iterator it = meshView.rbegin(); it != meshView.rend(); it++) {
			Entity e{ m_Scene.Get(), *it };
			MeshComponent& meshComponent = e.GetComponent<MeshComponent>();
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
		LUCY_PROFILE_NEW_EVENT("RendererModule::OnRender");

		EditorCamera& camera = m_Scene->GetEditorCamera();
		auto vp = camera.GetVP();

		auto& cameraBuffer = m_GeometryPipeline->GetUniformBuffers<VulkanUniformBuffer>("LucyCamera");
		cameraBuffer->SetData((uint8_t*)&vp, sizeof(vp));

		auto& cameraBufferID = m_IDPipeline->GetUniformBuffers<VulkanUniformBuffer>("LucyCamera");
		cameraBufferID->SetData((uint8_t*)&vp, sizeof(vp));

		Renderer::UpdateDescriptorSets(m_GeometryPipeline);
		Renderer::UpdateDescriptorSets(m_IDPipeline);

		Renderer::RenderScene();
	}

	void RendererModule::End() {
		LUCY_PROFILE_NEW_EVENT("RendererModule::End");

		RenderContextResultCodes result = Renderer::EndScene();
		if (result == RenderContextResultCodes::ERROR_OUT_OF_DATE_KHR || result == RenderContextResultCodes::SUBOPTIMAL_KHR)
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
		LUCY_PROFILE_NEW_EVENT("RendererModule::Wait");

		Renderer::WaitForDevice();
	}

	void RendererModule::OnWindowResize() {
		LUCY_PROFILE_NEW_EVENT("RendererModule::OnWindowResize");

		Renderer::OnWindowResize();
		OnViewportResize();
	}

	void RendererModule::OnViewportResize() {
		auto& [viewportWidth, viewportHeight] = Renderer::GetViewportArea();
		m_GeometryPipeline->Recreate(viewportWidth, viewportHeight);
		m_IDPipeline->Recreate(viewportWidth, viewportHeight);
	}
}
