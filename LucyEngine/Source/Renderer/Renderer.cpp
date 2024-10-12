#include "lypch.h"
#include <bitset>

#include "Renderer.h"
#include "Renderer/VulkanRenderer.h"
#include "RenderPass.h"

#include "Scene/Entity.h"

#include "RenderGraph/RenderGraph.h"

#include "Events/EventHandler.h"

#include "Image/Image.h"

#include "Memory/Buffer/Vulkan/VulkanFrameBuffer.h"

#include "Pipeline/ComputePipeline.h"

namespace Lucy {

	void Renderer::Init(RendererConfiguration config, const Ref<Window>& window) {
		s_Config = config;

		s_Renderer = RendererBackend::Create(config, window);
		s_Renderer->Init();

		const auto& device = GetRenderDevice();
		PushShader(Shader::Create("LucyPBR", "Assets/Shaders/LucyPBR.glsl", device));
		PushShader(Shader::Create("LucyID", "Assets/Shaders/LucyID.glsl", device));
		PushShader(Shader::Create("LucyHDRSkybox", "Assets/Shaders/LucyHDRSkybox.glsl", device));
		PushShader(Shader::Create("LucyImageToHDRConverter", "Assets/Shaders/LucyImageToHDRConverter.glsl", device));
		PushShader(Shader::Create("LucyDepthOnly", "Assets/Shaders/LucyDepthOnly.glsl", device));

#if USE_COMPUTE_FOR_CUBEMAP_GEN
		PushShader(Shader::Create("LucyIrradianceGen", "Assets/Shaders/LucyIrradianceGen.comp", device));
		PushShader(Shader::Create("LucyPrefilterGen", "Assets/Shaders/LucyPrefilterGen.comp", device));
#else
		PushShader(Shader::Create("LucyIrradianceGen", "Assets/Shaders/LucyIrradianceGen.glsl", device));
		PushShader(Shader::Create("LucyPrefilterGen", "Assets/Shaders/LucyPrefilterGen.glsl", device));
#endif

		s_RenderGraph = Memory::CreateRef<RenderGraph>();
		s_PipelineManager = Memory::CreateUnique<PipelineManager>(GetRenderDevice());
		s_MaterialManager = Memory::CreateUnique<MaterialManager>(s_Shaders);

		EnqueueToRenderThread([](const Ref<RenderDevice>& device) {
			static ImageCreateInfo blankCubeCreateInfo;
			blankCubeCreateInfo.Width = 1024;
			blankCubeCreateInfo.Height = 1024;
			blankCubeCreateInfo.Format = ImageFormat::R32G32B32A32_SFLOAT;
			blankCubeCreateInfo.ImageType = ImageType::TypeCubeColor;
			blankCubeCreateInfo.Parameter.U = ImageAddressMode::REPEAT;
			blankCubeCreateInfo.Parameter.V = ImageAddressMode::REPEAT;
			blankCubeCreateInfo.Parameter.W = ImageAddressMode::REPEAT;
			blankCubeCreateInfo.Parameter.Mag = ImageFilterMode::LINEAR;
			blankCubeCreateInfo.Parameter.Min = ImageFilterMode::LINEAR;
			blankCubeCreateInfo.GenerateSampler = true;

			s_BlankCubeHandle = device->CreateImage(blankCubeCreateInfo);
		});

		static std::vector<float> vertices = {
			//back face
			-1.0f, -1.0f, -1.0f,
			1.0f, 1.0f, -1.0f,
			1.0f, -1.0f, -1.0f,
			1.0f, 1.0f, -1.0f,
			-1.0f, -1.0f, -1.0f,
			-1.0f, 1.0f, -1.0f,
			// front face
			-1.0f, -1.0f, 1.0f,
			1.0f, -1.0f, 1.0f,
			1.0f, 1.0f, 1.0f,
			1.0f, 1.0f, 1.0f,
			-1.0f, 1.0f, 1.0f,
			-1.0f, -1.0f, 1.0f,
			// left face
			-1.0f, 1.0f, 1.0f,
			-1.0f, 1.0f, -1.0f,
			-1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f, 1.0f,
			-1.0f, 1.0f, 1.0f,
			// right face
			1.0f, 1.0f, 1.0f,
			1.0f, -1.0f, -1.0f,
			1.0f, 1.0f, -1.0f,
			1.0f, -1.0f, -1.0f,
			1.0f, 1.0f, 1.0f,
			1.0f, -1.0f, 1.0f,
			// bottom face
			-1.0f, -1.0f, -1.0f,
			1.0f, -1.0f, -1.0f,
			1.0f, -1.0f, 1.0f,
			1.0f, -1.0f, 1.0f,
			-1.0f, -1.0f, 1.0f,
			-1.0f, -1.0f, -1.0f,
			// top face
			-1.0f, 1.0f, -1.0f,
			1.0f, 1.0f, 1.0f,
			1.0f, 1.0f, -1.0f,
			1.0f, 1.0f, 1.0f,
			-1.0f, 1.0f, -1.0f,
			-1.0f, 1.0f, 1.0f
		};

		static std::vector<uint32_t> indices(vertices.size());
		for (uint32_t i = 0; i < indices.size(); i++)
			indices[i] = i;

		s_CubeMesh = Mesh::Create(vertices, indices);

		s_Renderer->FlushCommandQueue();
	}

	void Renderer::CompileRenderGraph() {
		s_RenderGraph->Compile();

		const auto& device = GetRenderDevice();
		auto& acyclicGraph = s_RenderGraph->GetAcyclicGraph();

		const auto CreateRenderPass = [&](RenderGraphPass* currentPass, const RGRenderTargetElements& rgRenderTargetElements, size_t maxLayerCount) {
			RenderPassLayout::Attachments colorAttachments;
			RenderPassLayout::Attachment depthAttachment;

			for (const RenderGraphResource& rgRenderTarget : rgRenderTargetElements) {
				auto image = s_RenderGraph->GetImageByRGResource(rgRenderTarget);
				const auto& imageData = s_RenderGraph->GetImageData(rgRenderTarget);
				bool isDepth = image->GetFormat() == ImageFormat::D32_SFLOAT;

				RenderGraphPass* renderTargetPass = acyclicGraph.FindOutputPassGivenResource(rgRenderTarget);
				bool usingRenderTargetsOfAnotherPass = currentPass != renderTargetPass;

				RenderPassLoadStoreAttachments loadStoreOp = imageData.LoadStoreAttachment;
				VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

				if (usingRenderTargetsOfAnotherPass) {
					loadStoreOp = RenderPassLoadStoreAttachments::LoadStore;
					initialLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR;
				}

				if (isDepth) {
					depthAttachment = {
						.Format = image->GetFormat(),
						.Samples = image->GetSamples(),
						.LoadStoreOperation = loadStoreOp,
						.StencilLoadStoreOperation = RenderPassLoadStoreAttachments::DontCareDontCare,
						.Initial = (RenderPassInternalLayout)(usingRenderTargetsOfAnotherPass ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED),
						.Final = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
						.Reference = RenderPassLayout::AttachmentReference{ VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }
					};
					continue;
				}

				colorAttachments.emplace_back(image->GetFormat(), image->GetSamples(),
											  loadStoreOp, RenderPassLoadStoreAttachments::DontCareDontCare,
											  initialLayout, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR,
											  RenderPassLayout::AttachmentReference{ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
			}

			RenderPassLayout passLayout {
				.ColorAttachments = colorAttachments,
				.DepthAttachment = depthAttachment
			};

			uint32_t viewMask = maxLayerCount == 1 ? 0x7FFFFFFFu : (1u << maxLayerCount) - 1;
			uint32_t correlationMask = maxLayerCount == 1 ? 0x7FFFFFFFu : (1u << 2) - 1;

			RenderPassCreateInfo passCreateInfo {
				.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f },
				.Layout = passLayout,
				.Multiview = {
					.ViewMask = viewMask,
					.CorrelationMask = correlationMask
				},
			};

			return device->CreateRenderPass(passCreateInfo);
		};

		const auto CreateFrameBuffer = [&](RenderGraphPass* currentPass, const RGRenderTargetElements& rgRenderTargetElements, auto renderPassHandle,
												uint32_t frameBufferWidth, uint32_t frameBufferHeight, bool isInFlight) {
			std::vector<RenderResourceHandle> imageBufferHandles;
			RenderResourceHandle depthImageHandle = InvalidRenderResourceHandle;

			for (const auto& rgRenderTarget : rgRenderTargetElements) {
				const auto& image = s_RenderGraph->GetImageByRGResource(rgRenderTarget);
				const auto& imageData = s_RenderGraph->GetImageData(rgRenderTarget);
				bool isDepth = image->GetFormat() == ImageFormat::D32_SFLOAT;

				RenderGraphPass* renderTargetPass = acyclicGraph.FindOutputPassGivenResource(rgRenderTarget);
				LUCY_ASSERT(renderTargetPass, "Pass cannot be found!");
				
				//this means that we are using render target of a another pass
				//if this is the case, just reuse the framebuffer and do not create a another one
				bool usingRenderTargetsOfAnotherPass = currentPass != renderTargetPass;
				if (usingRenderTargetsOfAnotherPass) {
					auto [renderPassHandle, frameBufferHandle] = s_RenderFrameHandleMap.at(renderTargetPass->GetName());
					return frameBufferHandle;
				}

				if (isDepth) {
					depthImageHandle = imageData.ResourceHandle;
					continue;
				}
				imageBufferHandles.push_back(imageData.ResourceHandle);
			}

			FrameBufferCreateInfo frameBufferCreateInfo{
				.Width = frameBufferWidth,
				.Height = frameBufferHeight,
				.IsInFlight = isInFlight,
				.RenderPassHandle = renderPassHandle,
				.ImageBufferHandles = imageBufferHandles,
				.DepthImageHandle = depthImageHandle
			};

			return device->CreateFrameBuffer(frameBufferCreateInfo);
		};

		const auto CreateGraphicsPipeline = [](const char* shaderName, const char* passName, const char* pipelineName, Rasterization rasterizationConfig = {}, DepthConfiguration depthConfig = {}) {
			if (!s_Shaders.contains(shaderName)) {
				LUCY_WARN("Shader '{0}' cannot be found while creating graphics pipeline '{1}' for pass '{2}'!",
					shaderName, pipelineName, passName);
				return;
			}
			const auto& shader = s_Shaders.at(shaderName);
			if (!s_RenderFrameHandleMap.contains(passName)) {
				LUCY_WARN("Frame handles for pass '{0}' cannot be found that uses shader '{1}' and tries to create graphics pipeline '{2}'!",
					passName, shaderName, pipelineName);
				return;
			}
			auto [renderPassHandle, frameBufferHandle] = s_RenderFrameHandleMap.at(passName);
			s_PipelineManager->CreateGraphicsPipeline(pipelineName, GraphicsPipelineCreateInfo{
				.Rasterization = rasterizationConfig,
				.DepthConfiguration = depthConfig,
				.VertexShaderLayout = shader->GetVertexShaderLayout(),
				.RenderPassHandle = renderPassHandle,
				.Shader = shader,
			});
		};

		const auto CreateComputePipeline = [](const char* shaderName, const char* pipelineName) {
			const auto& shader = s_Shaders.at(shaderName);
			if (!s_Shaders.contains(shaderName)) {
				LUCY_WARN("Shader '{0}' cannot be found while creating compute pipeline '{1}'!",
					shaderName, pipelineName);
				return;
			}
			s_PipelineManager->CreateComputePipeline(pipelineName, ComputePipelineCreateInfo{
				.Shader = shader->As<ComputeShader>(),
			});
		};

		for (const auto& node : acyclicGraph) {
			RenderGraphPass* pass = node.Pass;
			auto [viewportWidth, viewportHeight] = pass->GetViewportArea();

			if (viewportWidth == 0 && viewportHeight == 0)
				continue;

			const RGRenderTargetElements& rgRenderTargets = pass->GetRenderTargets();
			auto& maxLayeredRGRenderTarget = *std::ranges::max_element(rgRenderTargets, [&](const RenderGraphResource& a, const RenderGraphResource& b) {
				return s_RenderGraph->GetImageByRGResource(a)->GetLayerCount() < s_RenderGraph->GetImageByRGResource(b)->GetLayerCount();
			});

			auto renderPassHandle = CreateRenderPass(pass, rgRenderTargets, s_RenderGraph->GetImageByRGResource(maxLayeredRGRenderTarget)->GetLayerCount());
			auto frameBufferHandle = CreateFrameBuffer(pass, rgRenderTargets, renderPassHandle,
													   viewportWidth, viewportHeight,
													   pass->IsInFlightMode());
			s_RenderFrameHandleMap.try_emplace(pass->GetName(), RenderFrameHandles{ renderPassHandle, frameBufferHandle });
		}

		CreateGraphicsPipeline("LucyPBR", "PBRGeometryPass", "PBRGeometryPipeline", Rasterization{ .DisableBackCulling = true, .CullingMode = CullingMode::None });
		CreateGraphicsPipeline("LucyID", "IDPass", "IDPipeline");
		CreateGraphicsPipeline("LucyHDRSkybox", "CubemapPass", "SkyboxPipeline", Rasterization{ .DisableBackCulling = true, .CullingMode = CullingMode::None }, DepthConfiguration{ .DepthCompareOp = DepthCompareOp::LessOrEqual });
		CreateGraphicsPipeline("LucyImageToHDRConverter", "HDRImageToLayeredImage", "HDRImageToLayeredImageConvertPipeline");
		CreateGraphicsPipeline("LucyDepthOnly", "DepthOnlyPass", "DepthOnlyPipeline", Rasterization{ .DisableBackCulling = true, .CullingMode = CullingMode::None }, DepthConfiguration{ .DepthClampEnable = true, .DepthCompareOp = DepthCompareOp::LessOrEqual });
		CreateComputePipeline("LucyIrradianceGen", "IrradianceComputePipeline");

		GetRenderDevice()->CreateDeviceQueries(s_PipelineManager->GetAllPipelineCount(), s_RenderGraph->GetPassCount());
	}

	void Renderer::ImportExternalRenderGraphResource(const RenderGraphResource& renderGraphResource, RenderResourceHandle renderResourceHandle) {
		LUCY_PROFILE_NEW_EVENT("Renderer::ImportExternalRenderGraphResource");
		s_RenderGraph->ImportExternalResource(renderGraphResource, renderResourceHandle);
	}

	void Renderer::ImportExternalRenderGraphTransientResource(const RenderGraphResource& renderGraphResource, RenderResourceHandle renderResourceHandle) {
		LUCY_PROFILE_NEW_EVENT("Renderer::ImportExternalRenderGraphTransientResource");
		s_RenderGraph->ImportExternalTransientResource(renderGraphResource, renderResourceHandle);
	}

	void Renderer::ExecuteRenderGraph() {
		LUCY_PROFILE_NEW_EVENT("Renderer::ExecuteRenderGraph");
		s_RenderGraph->Execute();
	}

	void Renderer::Flush() {
		LUCY_PROFILE_NEW_EVENT("Renderer::Flush");
		s_RenderGraph->Flush();
	}

	RenderContextResultCodes Renderer::WaitAndPresent() {
		LUCY_PROFILE_NEW_EVENT("Renderer::WaitAndPresent");
		s_MaterialManager->UpdateMaterialsIfNecessary();
		return s_Renderer->WaitAndPresent();
	}

	void Renderer::Destroy() {
		EnqueueResourceDestroy(s_BlankCubeHandle);

		for (auto& [renderPassHandle, frameBufferHandle] : s_RenderFrameHandleMap | std::views::values) {
			EnqueueResourceDestroy(renderPassHandle);
			EnqueueResourceDestroy(frameBufferHandle);
		}
		
		s_PipelineManager->RTDestroyAll();
		s_MaterialManager->RTDestroyAll();

		s_CubeMesh->Destroy();
		DestroyAllShaders();
		s_Renderer->Destroy();
	}

	void Renderer::WaitForDevice() {
		GetRenderDevice()->WaitForDevice();
	}

	bool Renderer::IsOnRenderThread() {
		auto renderThread = s_Renderer->GetRenderThread();
		if (!renderThread) //threading policy is single-threaded
			return true;
		return renderThread->IsOnRenderThread();
	}

	Ref<Image> Renderer::GetOutputOfPass(const char* name) {
		LUCY_ASSERT(s_RenderFrameHandleMap.contains(name), "GetOutputOfPass failed because no pass with the name of {0} could be found!", name);
		const auto& device = GetRenderDevice();
		const auto& frameBufferHandle = s_RenderFrameHandleMap.at(name).FrameBufferHandle;
		const auto& frameBuffer = device->AccessResource<FrameBuffer>(frameBufferHandle);
		if (GetRenderArchitecture() == RenderArchitecture::Vulkan)
			return device->AccessResource<Image>(frameBuffer->As<VulkanFrameBuffer>()->GetImageHandles()[GetCurrentFrameIndex()]);
		LUCY_ASSERT(false);
		return nullptr;
	}

	void Renderer::DirectCopyBuffer(VkBuffer& stagingBuffer, VkBuffer& buffer, VkDeviceSize size) {
		s_Renderer->As<VulkanRenderer>()->DirectCopyBuffer(stagingBuffer, buffer, size);
	}

	void Renderer::SubmitImmediateCommand(std::function<void(VkCommandBuffer)>&& func) {
		s_Renderer->As<VulkanRenderer>()->SubmitImmediateCommand(std::move(func));
	}

	void Renderer::EnqueueToRenderThread(RenderCommandFunc&& func) {
		s_Renderer->EnqueueToRenderThread(std::move(func));
	}

	void Renderer::EnqueueResourceDestroy(RenderResourceHandle& handle) {
		s_Renderer->EnqueueResourceDestroy(handle);
	}

	void Renderer::InitializeImGui() {
		s_Renderer->InitializeImGui();
	}

	void Renderer::RenderImGui() {
		s_Renderer->RenderImGui();
	}

	bool Renderer::IsValidRenderResource(RenderResourceHandle handle) {
		bool isValid = (handle != InvalidRenderResourceHandle); //check if the handle is invalid
		isValid &= GetRenderDevice()->IsValidResource(handle);
		return isValid;
	}

	void Renderer::RTReloadShader(const std::string& name) {
		const auto& device = GetRenderDevice();
		device->WaitForQueue(TargetQueueFamily::Graphics);

		const Ref<Shader>& shader = s_Shaders.at(name);
		shader->RTDestroyResource(device);
		shader->RTLoad(device, true);

		s_PipelineManager->RTRecreateAllPipelinesDependentOnShader(shader);
	}

	void Renderer::SubmitToRender(RenderGraphPass& pass) {
		if (!s_RenderFrameHandleMap.contains(pass.GetName())) {
			s_Renderer->SubmitToCompute(pass);
			return;
		}
		const auto& [renderPassHandle, frameBufferHandle] = s_RenderFrameHandleMap.at(pass.GetName());
		s_Renderer->SubmitToRender(pass, renderPassHandle, frameBufferHandle);
	}

	void Renderer::OnEvent(Event& evt) {
		LUCY_PROFILE_NEW_EVENT("Renderer::OnEvent");
		EventHandler::AddListener<SwapChainResizeEvent>(evt, []([[maybe_unused]] const SwapChainResizeEvent& e) {
			OnWindowResize();
		});

		EventHandler::AddListener<ViewportAreaResizeEvent>(evt, [](const ViewportAreaResizeEvent& e) {
			auto newWidth = e.GetWidth();
			auto newHeight = e.GetHeight();

			OnViewportResize();
			
			{
				const auto& [renderPassHandle, frameBufferHandle] = s_RenderFrameHandleMap.at("PBRGeometryPass");
				AccessResource<FrameBuffer>(frameBufferHandle)->RTRecreate(newWidth, newHeight);
			}

			{
				const auto& [renderPassHandle, frameBufferHandle] = s_RenderFrameHandleMap.at("IDPass");
				AccessResource<FrameBuffer>(frameBufferHandle)->RTRecreate(newWidth, newHeight);
			}
		});

		EventHandler::AddListener<EntityPickedEvent>(evt, [](const EntityPickedEvent& e) {
			auto scene = e.GetScene();
			auto id = OnMousePicking(e);
			if (id == glm::vec3(-1.0))
				return;
			e.GetEntity() = scene->GetEntityByMeshID(id);
		});
	}

	void Renderer::OnWindowResize() {
		LUCY_PROFILE_NEW_EVENT("Renderer::OnWindowResize");
		s_Renderer->OnWindowResize();
	}

	void Renderer::OnViewportResize() {
		LUCY_PROFILE_NEW_EVENT("Renderer::OnViewportResize");
		s_Renderer->OnViewportResize();
	}

	glm::vec3 Renderer::OnMousePicking(const EntityPickedEvent& e) {
		LUCY_PROFILE_NEW_EVENT("Renderer::OnMousePicking");
		const auto& device = GetRenderDevice();
		auto frameBufferHandle = s_RenderFrameHandleMap.at("IDPass").FrameBufferHandle;
		const auto& frameBuffer = device->AccessResource<FrameBuffer>(frameBufferHandle);
		if (GetRenderArchitecture() == RenderArchitecture::Vulkan)
			return s_Renderer->OnMousePicking(e, device->AccessResource<Image>(frameBuffer->As<VulkanFrameBuffer>()->GetImageHandles()[GetCurrentFrameIndex()]));
		return glm::vec3(-1.0f);
	}

	void Renderer::PushShader(Ref<Shader> shader) {
		const auto& name = shader->GetName();
		LUCY_ASSERT(!s_Shaders.contains(name), "Creating shader that already exists!");
		s_Shaders.try_emplace(name, shader);
	}

	void Renderer::DestroyAllShaders() {
		for (const auto& shader : s_Shaders | std::views::values)
			shader->RTDestroyResource(GetRenderDevice());
	}
}