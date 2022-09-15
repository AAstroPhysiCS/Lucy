#include "lypch.h"
#include "RendererModule.h"
#include "Renderer/Renderer.h"

#include "Events/EventDispatcher.h"
#include "Events/WindowEvent.h"

#include "Context/VulkanSwapChain.h"

#include "Memory/Buffer/Vulkan/VulkanFrameBuffer.h"
#include "Memory/Buffer/Vulkan/VulkanUniformBuffer.h"

#include "Scene/Entity.h"

#include "Shader/ShaderLibrary.h"

namespace Lucy {

	/* --- Individual Resource Handles --- */
	static CommandResourceHandle g_GeometryPassHandle;
	static CommandResourceHandle g_IDPassHandle;

	RendererModule::RendererModule(RenderArchitecture arch, Ref<Window> window, Ref<Scene> scene)
		: Module(window, scene) {
		Renderer::Init(arch, window);

#ifdef LUCY_DEBUG
		window->SetTitle(fmt::format("{0} - Windows x64 Debug {1}", m_Window->GetTitle(),
						 arch == RenderArchitecture::Vulkan ? "Vulkan" : "DirectX12").c_str());
#else
		window->SetTitle(fmt::format("{0} - Windows x64 Release {1}", m_Window->GetTitle(),
						 arch == RenderArchitecture::Vulkan ? "Vulkan" : "DirectX12").c_str());
#endif

		const auto& [viewportWidth, viewportHeight] = Renderer::GetViewportArea();

		const ShaderLibrary& shaderLibrary = ShaderLibrary::Get();
		const auto& pbrShader = shaderLibrary.GetShader("LucyPBR");
		const auto& idShader = shaderLibrary.GetShader("LucyID");

		const uint32_t maxFramesInFlight = Renderer::GetMaxFramesInFlight();

#pragma region GeometryPipeline

		ImageCreateInfo geometryTextureCreateInfo = {
			.Width = viewportWidth,
			.Height = viewportHeight,
			.ImageType = ImageType::Type2DColor,
			.Format = ImageFormat::R8G8B8A8_UNORM,
			.Parameter {
				.U = ImageAddressMode::REPEAT,
				.V = ImageAddressMode::REPEAT,
				.W = ImageAddressMode::REPEAT,
				.Min = ImageFilterMode::LINEAR,
				.Mag = ImageFilterMode::LINEAR,
			},
			.ImGuiUsage = true,
			.GenerateSampler = true
		};

		ImageCreateInfo depthImageCreateInfo {
			.Width = geometryTextureCreateInfo.Width,
			.Height = geometryTextureCreateInfo.Height,
			.ImageType = ImageType::Type2DDepth,
			.Format = ImageFormat::D32_SFLOAT,
		};

		RenderPassLayout geometryPassLayout{
			.ColorAttachments = {
				RenderPassLayout::Attachment {
					.Format = geometryTextureCreateInfo.Format,
					.Samples = 1,
					.LoadOp = RenderPassLoadOp::Clear,
					.StoreOp = RenderPassStoreOp::Store,
					.StencilLoadOp = RenderPassLoadOp::DontCare,
					.StencilStoreOp = RenderPassStoreOp::DontCare,
					.Initial = VK_IMAGE_LAYOUT_UNDEFINED,
					.Final = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR,
					.Reference = { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }
				},
			},
			.DepthAttachment = RenderPassLayout::Attachment {
				.Format = depthImageCreateInfo.Format,
				.Samples = 1,
				.LoadOp = RenderPassLoadOp::Clear,
				.StoreOp = RenderPassStoreOp::Store,
				.StencilLoadOp = RenderPassLoadOp::DontCare,
				.StencilStoreOp = RenderPassStoreOp::DontCare,
				.Initial = VK_IMAGE_LAYOUT_UNDEFINED,
				.Final = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				.Reference = { VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }
			}
		};

		RenderPassCreateInfo geometryPassCreateInfo {
			.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f },
			.Layout = geometryPassLayout,
		};

		FrameBufferCreateInfo geometryFrameBufferCreateInfo {
			.Width = viewportWidth,
			.Height = viewportHeight,
			.IsInFlight = true,
			.ImageBuffers {
				Image::Create(geometryTextureCreateInfo)
			},
			.DepthImage = Image::Create(depthImageCreateInfo)
		};

		GraphicsPipelineCreateInfo geometryPipelineCreateInfo {
			.Topology = Topology::TRIANGLES,
			.Rasterization = {
				.DisableBackCulling = true,
				.CullingMode = CullingMode::None,
				.LineWidth = 1.0f,
				.PolygonMode = PolygonMode::FILL
			},
			.VertexShaderLayout {
				{ "a_Pos", ShaderDataSize::Float3 },
				{ "a_TextureCoords", ShaderDataSize::Float2 },
				{ "a_ID", ShaderDataSize::Float3 },
				{ "a_Normals", ShaderDataSize::Float3 },
				{ "a_Tangents", ShaderDataSize::Float3 },
				{ "a_BiTangents", ShaderDataSize::Float3 }
			},
			.Shader = pbrShader
		};

		Ref<RenderPass> geometryRenderPass = RenderPass::Create(geometryPassCreateInfo);
		geometryPipelineCreateInfo.RenderPass = geometryRenderPass;
		geometryFrameBufferCreateInfo.RenderPass = geometryRenderPass;

		Ref<FrameBuffer> geometryFrameBuffer = FrameBuffer::Create(geometryFrameBufferCreateInfo);
		geometryPipelineCreateInfo.FrameBuffer = geometryFrameBuffer;

		m_GeometryPipeline = GraphicsPipeline::Create(geometryPipelineCreateInfo);
#pragma endregion GeometryPipeline

#pragma region IDPipeline

		ImageCreateInfo idTextureCreateInfo = geometryTextureCreateInfo;
		idTextureCreateInfo.Flags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		idTextureCreateInfo.ImGuiUsage = false;

		RenderPassLayout idPassLayout = geometryPassLayout;
		idPassLayout.ColorAttachments[0].Format = idTextureCreateInfo.Format;
		idPassLayout.ColorAttachments[0].Final = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		
		RenderPassCreateInfo idPassCreateInfo = geometryPassCreateInfo;
		idPassCreateInfo.Layout = idPassLayout;
		Ref<RenderPass> idRenderPass = RenderPass::Create(idPassCreateInfo);

		FrameBufferCreateInfo idFrameBufferCreateInfo = geometryFrameBufferCreateInfo;
		idFrameBufferCreateInfo.RenderPass = idRenderPass;
		idFrameBufferCreateInfo.ImageBuffers = {
			Image::Create(idTextureCreateInfo)
		};
		idFrameBufferCreateInfo.DepthImage = Image::Create(depthImageCreateInfo);

		Ref<FrameBuffer> idFrameBuffer = FrameBuffer::Create(idFrameBufferCreateInfo);

		GraphicsPipelineCreateInfo idPipelineCreateInfo = geometryPipelineCreateInfo;
		idPipelineCreateInfo.Shader = idShader;
		idPipelineCreateInfo.RenderPass = idRenderPass;
		idPipelineCreateInfo.FrameBuffer = idFrameBuffer;

		m_IDPipeline = GraphicsPipeline::Create(idPipelineCreateInfo);
#pragma endregion IDPipeline

		g_GeometryPassHandle = Renderer::CreateCommandResource(GeometryPass, m_GeometryPipeline);
		g_IDPassHandle = Renderer::CreateCommandResource(IDPass, m_IDPipeline);
	}

	void RendererModule::Begin() {
		LUCY_PROFILE_NEW_EVENT("RendererModule::Begin");

		const auto& directionalLightView = m_Scene->View<DirectionalLightComponent>();
		for (auto& entity : directionalLightView) {
			Entity e{ m_Scene.Get(), entity };
			DirectionalLightComponent& lightComponent = e.GetComponent<DirectionalLightComponent>();

			const auto& lightningAttributes = m_GeometryPipeline->GetUniformBuffers<VulkanUniformBuffer>("LucyLightningValues");
			lightningAttributes->SetData((uint8_t*)&lightComponent, sizeof(DirectionalLightComponent));
		}

		const auto& meshView = m_Scene->View<MeshComponent>();
		for (entt::sparse_set::reverse_iterator it = meshView.rbegin(); it != meshView.rend(); it++) {
			Entity e{ m_Scene.Get(), *it };
			MeshComponent& meshComponent = e.GetComponent<MeshComponent>();
			if (!meshComponent.IsValid())
				continue;

			const Ref<Mesh>& mesh = meshComponent.GetMesh();

			auto& materials = mesh->GetMaterials();
			for (Submesh& submesh : mesh->GetSubmeshes()) {
				const Ref<Material>& material = materials[submesh.MaterialIndex];
				material->Update(m_GeometryPipeline);
			}

			//High priority stuff must be called earlier than low, since those meshes get render first.
			//I won't be sorting the render commands accordingly, the sort must happen here (for performance reason)
			Renderer::EnqueueCommand<StaticMeshRenderCommand>(g_GeometryPassHandle, Priority::LOW, meshComponent.GetMesh(), e.GetComponent<TransformComponent>().GetMatrix());
			Renderer::EnqueueCommand<StaticMeshRenderCommand>(g_IDPassHandle, Priority::LOW, meshComponent.GetMesh(), e.GetComponent<TransformComponent>().GetMatrix());
		}

		Renderer::BeginScene(m_Scene);
	}

	void RendererModule::OnRender() {
		LUCY_PROFILE_NEW_EVENT("RendererModule::OnRender");

		EditorCamera& camera = m_Scene->GetEditorCamera();
		auto vp = camera.GetVP();

		auto cameraBuffer = m_GeometryPipeline->GetUniformBuffers<VulkanUniformBuffer>("LucyCamera");
		cameraBuffer->SetData((uint8_t*)&vp, sizeof(vp));

		auto cameraBufferID = m_IDPipeline->GetUniformBuffers<VulkanUniformBuffer>("LucyCamera");
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
		const auto& [viewportWidth, viewportHeight] = Renderer::GetViewportArea();
		m_GeometryPipeline->Recreate(viewportWidth, viewportHeight);
		m_IDPipeline->Recreate(viewportWidth, viewportHeight);
	}
}
