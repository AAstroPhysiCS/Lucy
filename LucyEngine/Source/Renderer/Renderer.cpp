#include "lypch.h"
#include <array>
#include <future>

#include "Core/Timer.h"
#include "Core/Application.h"

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

		if (config.ThreadingPolicy == ThreadingPolicy::Multithreaded) {
			static std::promise<void> renderThreadInitPromise;
			static auto renderThreadInitFuture = renderThreadInitPromise.get_future();

			s_RenderThread = new RenderThread(
				RunnableThreadCreateInfo{
				   .Name = "LucyRenderThread",
				   .Affinity = ThreadApplicationAffinityIncremental,
				   .Priority = ThreadPriority::Highest
				},
				RenderThreadCreateInfo{
					.Window = window,
					.Config = config,
					.InitPromise = renderThreadInitPromise,
				}
			);

			s_RenderThread->Start();

			renderThreadInitFuture.wait();
		} else {
			s_Backend = RendererBackend::Create(config, window);
			s_Backend->Init();
		}

		LUCY_ASSERT(s_Backend, "RendererBackend is nullptr!");

		const auto& device = GetRenderDevice();
		
		s_ShaderManager.InitializeShaders(device);

		s_RenderGraph = Memory::CreateRef<RenderGraph>(s_Config.RenderArchitecture, s_Backend->GetRenderDevice());
		s_PipelineManager = Memory::CreateUnique<PipelineManager>(GetRenderDevice());
		s_MaterialManager = Memory::CreateUnique<MaterialManager>(s_PipelineManager);

		EnqueueToRenderCommandQueue([](const Ref<RenderDevice>& device) {
			static ImageCreateInfo blankCubeCreateInfo;
			blankCubeCreateInfo.Width = 1024;
			blankCubeCreateInfo.Height = 1024;
			blankCubeCreateInfo.Format = ImageFormat::R32G32B32A32_SFLOAT;
			blankCubeCreateInfo.ImageType = ImageType::TypeCube;
			blankCubeCreateInfo.ImageUsage = ImageUsage::AsColorTransferAttachment;
			blankCubeCreateInfo.GenerateSampler = true;

			s_BlankCubeHandle = device->CreateImage(blankCubeCreateInfo);

			static ImageCreateInfo blankArrayCreateInfo = blankCubeCreateInfo;
			blankArrayCreateInfo.ImageType = ImageType::Type2D;
			blankArrayCreateInfo.Layers = 6;
			s_BlankArrayHandle = device->CreateImage(blankArrayCreateInfo);
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

		if (config.ThreadingPolicy == ThreadingPolicy::Singlethreaded)
			s_Backend->FlushCommandQueue();
	}

	void Renderer::CompileRenderGraph() {
		s_RenderGraph->Build();

		const auto& device = GetRenderDevice();
		auto& acyclicGraph = s_RenderGraph->GetAcyclicGraph();

		const auto CreateRenderPass = [&](RenderGraphPass* currentPass, const RGRenderTargetElements& rgRenderTargetElements, size_t maxLayerCount) {
			RenderPassLayout::Attachments colorAttachments;
			RenderPassLayout::Attachment depthAttachment;

			for (const RenderGraphResource& rgRenderTarget : rgRenderTargetElements) {
				auto image = s_RenderGraph->GetImageByRGResource(rgRenderTarget);
				bool isDepth = image->GetFormat() == ImageFormat::D32_SFLOAT;

				RenderGraphPass* renderTargetPass = acyclicGraph.FindOutputPassGivenResource(rgRenderTarget);
				bool usingRenderTargetsOfAnotherPass = currentPass != renderTargetPass;

				RenderPassLoadStoreAttachments loadStoreOp = s_RenderGraph->GetLoadStoreAttachmentsByRGResource(rgRenderTarget);
				VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

				if (usingRenderTargetsOfAnotherPass) {
					loadStoreOp = RenderPassLoadStoreAttachments::LoadStore;
					initialLayout = image->As<VulkanImage>()->GetCurrentLayout();
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
											  initialLayout, image->As<VulkanImage>()->GetCurrentLayout(),
											  RenderPassLayout::AttachmentReference{ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
			}

			RenderPassLayout passLayout {
				.ColorAttachments = colorAttachments,
				.DepthAttachment = depthAttachment
			};

			uint32_t viewMask = maxLayerCount == 1 ? 0x7FFFFFFFu : (1u << maxLayerCount) - 1;
			uint32_t correlationMask = maxLayerCount == 1 ? 0x7FFFFFFFu : (1u << 2) - 1;

			RenderPassCreateInfo passCreateInfo {
				.ClearColor = currentPass->GetClearColor(),
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
				auto handle = s_RenderGraph->GetHandleByRGResource(rgRenderTarget);
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
					depthImageHandle = handle;
					continue;
				}
				imageBufferHandles.push_back(handle);
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

		const auto CreateGraphicsPipeline = [](const char* shaderName, const char* passName, const char* pipelineName, 
			Rasterization rasterizationConfig = {}, DepthConfiguration depthConfig = {}, BlendConfiguration blendConfig = {}) {
			if (!s_ShaderManager.HasShader(shaderName)) {
				LUCY_WARN("Shader '{0}' cannot be found while creating graphics pipeline '{1}' for pass '{2}'!",
					shaderName, pipelineName, passName);
				return;
			}
			const auto& shader = s_ShaderManager.GetShader(ShaderStageType::VertexAndFragment, shaderName);
			if (!s_RenderFrameHandleMap.contains(passName)) {
				LUCY_WARN("Frame handles for pass '{0}' cannot be found that uses shader '{1}' and tries to create graphics pipeline '{2}'!",
					passName, shaderName, pipelineName);
				return;
			}
			auto [renderPassHandle, frameBufferHandle] = s_RenderFrameHandleMap.at(passName);
			s_PipelineManager->CreateGraphicsPipeline(pipelineName, GraphicsPipelineCreateInfo{
				.Rasterization = rasterizationConfig,
				.DepthConfiguration = depthConfig,
				.BlendConfiguration = blendConfig,
				.RenderPassHandle = renderPassHandle,
				.Shader = shader,
			});
		};

		const auto CreateComputePipeline = [](const char* shaderName, const char* pipelineName) {
			if (!s_ShaderManager.HasShader(shaderName)) {
				LUCY_WARN("Shader '{0}' cannot be found while creating compute pipeline '{1}'!",
					shaderName, pipelineName);
				return;
			}
			const auto& shader = s_ShaderManager.GetShader(ShaderStageType::Compute, shaderName);
			s_PipelineManager->CreateComputePipeline(pipelineName, ComputePipelineCreateInfo{
				.Shader = shader->As<ComputeShader>(),
			});
		};

		for (const auto& node : acyclicGraph) {
			RenderGraphPass* pass = node.Pass;
			auto [viewportWidth, viewportHeight] = pass->GetViewportArea();

			if (viewportWidth == 0 && viewportHeight == 0) {
				LUCY_WARN("Viewport area of pass '{0}' is 0, skipping...", pass->GetName());
				continue;
			}

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

		TaskScheduler* taskScheduler = Application::GetTaskScheduler();
		{
			ScopedTimer timer("Pipeline Creation");

			struct RenderGraphPipelineCreateInfo {
				const char* ShaderName;
				const char* PassName;
				const char* PipelineName;
				Rasterization RasterizationConfig = {};
				DepthConfiguration DepthConfig = {};
				BlendConfiguration BlendConfig = {};
			};

#if !USE_COMPUTE_FOR_CUBEMAP_GEN
			constexpr size_t graphicsPipelineCount = 6;
#else
			constexpr size_t graphicsPipelineCount = 5;
#endif
			constexpr const std::array<RenderGraphPipelineCreateInfo, graphicsPipelineCount> graphicsPipelineCreateInfos = {
				// PBR Geometry Pipeline
				RenderGraphPipelineCreateInfo {
					.ShaderName = "LucyPBR",
					.PassName = "PBRGeometryPass",
					.PipelineName = "PBRGeometryPipeline",
					.RasterizationConfig = {.DisableBackCulling = true, .CullingMode = CullingMode::None}
				},
				// ID Pipeline
				{
					.ShaderName = "LucyID",
					.PassName = "IDPass",
					.PipelineName = "IDPipeline"
				},
				// Skybox Pipeline
				{
					.ShaderName = "LucyHDRSkybox",
					.PassName = "CubemapPass",
					.PipelineName = "SkyboxPipeline",
					.RasterizationConfig = {.DisableBackCulling = true, .CullingMode = CullingMode::None},
					.DepthConfig = {.DepthCompareOp = DepthCompareOp::LessOrEqual}
				},
				// HDR Converter Pipeline
				{
					.ShaderName = "LucyImageToHDRConverter",
					.PassName = "HDRImageToLayeredImage",
					.PipelineName = "HDRImageToLayeredImageConvertPipeline"
				},
				// VSM Pipeline
				{
					.ShaderName = "LucyVSM",
					.PassName = "VSMPass",
					.PipelineName = "VSMPipeline",
					.RasterizationConfig = {.DisableBackCulling = true, .CullingMode = CullingMode::None},
					.DepthConfig = {.DepthClipEnable = VK_FALSE, .DepthCompareOp = DepthCompareOp::LessOrEqual},
					.BlendConfig = {.BlendEnable = VK_FALSE}
				},
#if !USE_COMPUTE_FOR_CUBEMAP_GEN
				{
					.ShaderName = "LucyIrradianceGen",
					.PassName = "IrradiancePass",
					.PipelineName = "IrradiancePipeline",
				},
#endif
			};

#if USE_COMPUTE_FOR_CUBEMAP_GEN
			constexpr size_t computePipelineCount = 5;
#else
			constexpr size_t computePipelineCount = 4;
#endif
			constexpr const std::array<RenderGraphPipelineCreateInfo, computePipelineCount> computePipelineCreateInfos = {
#if USE_COMPUTE_FOR_CUBEMAP_GEN
				RenderGraphPipelineCreateInfo {
					.ShaderName = "LucyIrradianceGen",
					.PipelineName = "IrradianceComputePipeline"
				},
#endif
				RenderGraphPipelineCreateInfo {
					.ShaderName = "LucyBRDFLut",
					.PipelineName = "BRDFLutComputePipeline"
				},
				RenderGraphPipelineCreateInfo {
					.ShaderName = "LucyPrefilterGen",
					.PipelineName = "PrefilterComputePipeline"
				},
				RenderGraphPipelineCreateInfo {
					.ShaderName = "LucyBlur",
					.PipelineName = "VSMHorizontalBlurComputePipeline"
				},
				RenderGraphPipelineCreateInfo {
					.ShaderName = "LucyBlur",
					.PipelineName = "VSMVerticalBlurComputePipeline"
				},
			};

			static std::mutex pipelineMutex;

			taskScheduler->ScheduleBatch(TaskScheduler::Launch::Async, TaskPriority::High, [&](const TaskArgs& args, const TaskBatchArgs& batchArgs) {
				const auto& createInfo = graphicsPipelineCreateInfos[batchArgs.BatchIndex];
				std::unique_lock lock(pipelineMutex);
				CreateGraphicsPipeline(createInfo.ShaderName, createInfo.PassName, createInfo.PipelineName, createInfo.RasterizationConfig, createInfo.DepthConfig, createInfo.BlendConfig);
			}, graphicsPipelineCount, 1);

			taskScheduler->ScheduleBatch(TaskScheduler::Launch::Async, TaskPriority::High, [&](const TaskArgs& args, const TaskBatchArgs& batchArgs) {
				const auto& createInfo = computePipelineCreateInfos[batchArgs.BatchIndex];
				std::unique_lock lock(pipelineMutex);
				CreateComputePipeline(createInfo.ShaderName, createInfo.PipelineName);
			}, computePipelineCount, 1);

			taskScheduler->WaitForAllTasks();
		}

		device->CreatePipelineDeviceQueries(s_PipelineManager->GetGraphicsPipelineCount());
		device->CreateTimestampDeviceQueries(s_RenderGraph->GetPassCount());
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
		auto batches = s_RenderGraph->Execute();
		SubmitToRender(batches);
	}

	void Renderer::Flush() {
		LUCY_PROFILE_NEW_EVENT("Renderer::Flush");
		s_RenderGraph->Flush();
		s_MaterialManager->UpdateMaterialsIfNecessary();
	}

	RenderContextResultCodes Renderer::WaitAndPresent() {
		LUCY_PROFILE_NEW_EVENT("Renderer::WaitAndPresent");
		return s_Backend->WaitAndPresent();
	}

	void Renderer::Destroy() {
		WaitForDevice();

		EnqueueResourceDestroy(s_BlankCubeHandle);

		/*
		* Have to do this, since some passes can be utilizing the framebuffer of other passes, so before i delete them, i have to
		* exclude those passes out, in order to not delete the same resource twice.
		*/

		std::unordered_set<RenderResourceHandle> distinctRenderPassHandles;
		std::unordered_set<RenderResourceHandle> distinctFrameBufferHandles;
		distinctRenderPassHandles.reserve(s_RenderFrameHandleMap.size());
		distinctFrameBufferHandles.reserve(s_RenderFrameHandleMap.size());

		for (auto&& [rp, fb] : s_RenderFrameHandleMap | std::views::values) {
			distinctRenderPassHandles.insert(rp);
			distinctFrameBufferHandles.insert(fb);
		}

		auto DestroyResources = [](const auto& resourceSet) {
			for (auto resource : resourceSet) {
				EnqueueResourceDestroy(resource);
			}
		};

		DestroyResources(distinctRenderPassHandles);
		DestroyResources(distinctFrameBufferHandles);

		s_PipelineManager->DestroyAll();
		s_MaterialManager->DestroyAll();
		
		s_CubeMesh->Destroy();
		DestroyAllShaders();

		if (s_Config.ThreadingPolicy == ThreadingPolicy::Singlethreaded)
			s_Backend->Destroy();
		else
			s_RenderThread->SignalToShutdown();

		s_RenderThread->WaitToShutdown();
		delete s_RenderThread;
	}

	void Renderer::WaitForDevice() {
		s_Backend->GetRenderDevice()->WaitForDevice();
	}

	bool Renderer::IsOnRenderThread() {
		if (s_Config.ThreadingPolicy == ThreadingPolicy::Singlethreaded)
			return true;
		return s_RenderThread->IsOnRenderThread();
	}

	void Renderer::RTSetBackend(Ref<RendererBackend> backend) {
		LUCY_ASSERT(IsOnRenderThread(), "RTSetBackend is being called from the main thread!");
		s_Backend = backend;
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

	void Renderer::RTDirectCopyBuffer(VkBuffer& stagingBuffer, VkBuffer& buffer, VkDeviceSize size) {
		LUCY_ASSERT(IsOnRenderThread(), "RTDirectCopyBuffer is being called from the main thread!");
		s_Backend->As<VulkanRenderer>()->RTDirectCopyBuffer(stagingBuffer, buffer, size);
	}

	void Renderer::SubmitImmediateCommand(std::function<void(VkCommandBuffer)>&& func) {
		s_Backend->As<VulkanRenderer>()->SubmitImmediateCommand(std::move(func));
	}

	void Renderer::EnqueueToRenderCommandQueue(RenderCommandFunc&& func) {
		s_Backend->EnqueueToRenderCommandQueue(std::move(func));
	}

	void Renderer::EnqueueResourceDestroy(RenderResourceHandle& handle) {
		s_Backend->EnqueueResourceDestroy(handle);
	}

	void Renderer::InitializeImGui() {
		s_Backend->InitializeImGui();
	}

	bool Renderer::IsValidRenderResource(RenderResourceHandle handle) {
		bool isValid = (handle != InvalidRenderResourceHandle); //check if the handle is invalid
		isValid &= GetRenderDevice()->IsValidResource(handle);
		return isValid;
	}

	void Renderer::ReloadShader(const std::string& name) {
		EnqueueToRenderCommandQueue([name](const auto& device) {
			LUCY_ASSERT(IsOnRenderThread(), "ReloadShader is being called from the main thread!");
			device->WaitForQueue(TargetQueueFamily::Graphics);
			device->WaitForQueue(TargetQueueFamily::Compute);

			const auto& shaders = s_ShaderManager.GetShaderStageMap(name);
			for (const auto& shader : shaders | std::views::values)
				shader->RTDestroyResource(device);

			s_ShaderManager.ReloadShader(device, name);
		});

		s_PipelineManager->RTRecreateAllPipelinesDependentOnShader(name);
	}

	void Renderer::SubmitToRender(std::vector<ExecutionBatch>& batches) {
		LUCY_PROFILE_NEW_EVENT("Renderer::SubmitToRender");
		s_Backend->SubmitBatchesToRender(batches, s_RenderFrameHandleMap);
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
		s_Backend->OnWindowResize();
	}

	void Renderer::OnViewportResize() {
		LUCY_PROFILE_NEW_EVENT("Renderer::OnViewportResize");
		s_Backend->OnViewportResize();
	}

	glm::vec3 Renderer::OnMousePicking(const EntityPickedEvent& e) {
		LUCY_PROFILE_NEW_EVENT("Renderer::OnMousePicking");

		const auto& device = GetRenderDevice();
		auto frameBufferHandle = s_RenderFrameHandleMap.at("IDPass").FrameBufferHandle;
		const auto& frameBuffer = device->AccessResource<FrameBuffer>(frameBufferHandle);
		if (GetRenderArchitecture() == RenderArchitecture::Vulkan)
			return s_Backend->OnMousePicking(e, device->AccessResource<Image>(frameBuffer->As<VulkanFrameBuffer>()->GetImageHandles()[GetCurrentFrameIndex()]));
		return glm::vec3(-1.0f);
	}

	void Renderer::DestroyAllShaders() {
		EnqueueToRenderCommandQueue([](const auto& device){
			LUCY_ASSERT(IsOnRenderThread(), "DestroyAllShaders is being called from the main thread!");

			s_ShaderManager.DestroyAllShaders(device);
			s_ShaderManager.Destroy();
		});
	}
}