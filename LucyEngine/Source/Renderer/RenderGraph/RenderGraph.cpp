#include "lypch.h"
#include "RenderGraph.h"
#include "RenderGraphPass.h"
#include "RenderGraphResource.h"
#include "RenderGraphBuilder.h"

#include "Renderer/ExecutionBatch.h"

#include "Renderer/Image/VulkanImage.h"
#include "Renderer/Renderer.h"

namespace Lucy {
	
	RenderGraph::RenderGraph(RenderArchitecture arch, Ref<RenderDevice> device) 
		: m_RenderDevice(device) {
		switch (arch) {
			case RenderArchitecture::Vulkan:
				m_Compiler = Memory::CreateUnique<VulkanRenderGraphCompiler>(*this, device);
				break;
			default:
				LUCY_ASSERT(false, "Unsupported Render Architecture!");
		}
	}

	void RenderGraph::Build() {
		LUCY_PROFILE_NEW_EVENT("RenderGraph::Build");
		m_AcyclicGraph.Build();
		Update();
	}

	std::vector<ExecutionBatch> RenderGraph::Execute() {
		LUCY_PROFILE_NEW_EVENT("RenderGraph::Execute");
		return m_Compiler->Compile(CreateBatchesForRendering());
	}

	void RenderGraph::Flush() {
		LUCY_PROFILE_NEW_EVENT("RenderGraph::Flush");
		Update();
	}

	void RenderGraph::ImportExternalResource(const RenderGraphResource& rgResource, RenderDeviceResourceHandle handle, RGResourceData data) {
		m_Registry.ImportExternalResource(rgResource, handle, data);
	}

	void RenderGraph::ImportExternalResource(const RenderGraphResource& rgResource, const std::vector<RenderDeviceResourceHandle>& handles, RGResourceData data) {
		m_Registry.ImportExternalResource(rgResource, handles, data);
	}

	void RenderGraph::ImportExternalTransientResource(const RenderGraphResource& rgResource, RenderDeviceResourceHandle handle) {
		m_Registry.ImportExternalTransientResource(rgResource, handle);
	}

	void RenderGraph::DeclareImage(const RenderGraphResource& rgResource, const ImageCreateInfo& createInfo, RenderPassLoadStoreAttachments loadStoreAccessOp, bool isInFlightMode) {
		DeclareImage(rgResource, createInfo, loadStoreAccessOp, UndefinedRenderGraphResource, {}, RenderPassLoadStoreAttachments::NoneNone, isInFlightMode);
	}

	void RenderGraph::DeclareImage(const RenderGraphResource& rgResource, const ImageCreateInfo& createInfo, RenderPassLoadStoreAttachments loadStoreAccessOp, const RenderGraphResource& rgResourceDepth, const ImageCreateInfo& createDepthInfo, RenderPassLoadStoreAttachments loadStoreDepthAccessOp, bool isInFlightMode) {
		const uint32_t imageCount = isInFlightMode ? Renderer::GetMaxFramesInFlight() : 1;

		std::vector<RenderDeviceResourceHandle> imageHandles;
		imageHandles.reserve(imageCount);
		for (uint32_t i = 0; i < imageCount; i++)
			imageHandles.emplace_back(m_RenderDevice->CreateImage(createInfo, "Image " + rgResource.GetName()));

		m_Registry.DeclareImage(
			rgResource,
			imageHandles,
			RGImageData{
				.LoadStoreAttachment = loadStoreAccessOp,
				.IsDepth = false,
				.InFlightMode = isInFlightMode
			}
		);

		if (rgResourceDepth == UndefinedRenderGraphResource)
			return;

		std::vector<RenderDeviceResourceHandle> depthImageHandles;
		depthImageHandles.reserve(imageCount);
		for (uint32_t i = 0; i < imageCount; i++)
			depthImageHandles.emplace_back(m_RenderDevice->CreateImage(createDepthInfo, "Depth " + rgResourceDepth.GetName()));

		m_Registry.DeclareImage(
			rgResourceDepth,
			depthImageHandles,
			RGImageData{
				.LoadStoreAttachment = loadStoreDepthAccessOp,
				.IsDepth = true,
				.InFlightMode = isInFlightMode
			}
		);
	}

	void RenderGraph::DeclareBuffer(const RenderGraphResource& rgResource, const RenderDeviceBufferCreateInfo& createInfo, bool isInFlightMode) {
		const uint32_t bufferCount = isInFlightMode ? Renderer::GetMaxFramesInFlight() : 1;
		std::vector<RenderDeviceResourceHandle> bufferHandles;
		bufferHandles.reserve(bufferCount);
		for (uint32_t i = 0; i < bufferCount; i++)
			bufferHandles.emplace_back(m_RenderDevice->CreateDeviceAddressBuffer(createInfo));

		m_Registry.DeclareBuffer(
			rgResource,
			bufferHandles,
			RGBufferData {
				.InFlightMode = isInFlightMode
			}
		);
	}

	void RenderGraph::ReadExternalImage(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToRead) {
		if (m_Registry.Contains(rgResourceToRead)) {
			//const auto& image = device->AccessResource<Image>(externalResources.at(resource));
			ReadImage(currentPass, rgResourceToRead);
			return;
		}

		//see transient image comment section
		ImportExternalResource(rgResourceToRead, RenderDeviceResourceHandle{});
		ReadImage(currentPass, rgResourceToRead);
	}

	void RenderGraph::ReadExternalTransientImage(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToRead) {
		if (m_Registry.Contains(rgResourceToRead)) {
			//const auto& image = device->AccessResource<Image>(externalTransientResources.at(resource));
			ReadImage(currentPass, rgResourceToRead);
			return;
		}

		//this means that the resource is not ready for the pass => waits probably for an user input or something else.
		//despite this, import it, but give it an invalid render resource handle
		//rendergraph will cull passes that references this image, automatically
		//the invalid render resource handle will be replaced if user decides to load the image.
		ImportExternalTransientResource(rgResourceToRead, {});
		ReadImage(currentPass, rgResourceToRead);
	}

	void RenderGraph::WriteExternalImage(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToWrite) {
		if (m_Registry.Contains(rgResourceToWrite)) {
			//const auto& image = device->AccessResource<Image>(externalResources.at(resource));
			WriteImage(currentPass, rgResourceToWrite);
			return;
		}

		//see transient image comment section
		ImportExternalResource(rgResourceToWrite, RenderDeviceResourceHandle{});
		WriteImage(currentPass, rgResourceToWrite);
	}

	void RenderGraph::BindRenderTarget(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToBind, const RenderGraphResource& rgResourceDepthToBind) {
		BindRenderTarget(currentPass, rgResourceToBind);
		BindRenderTarget(currentPass, rgResourceDepthToBind);
	}

	void RenderGraph::BindRenderTarget(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToBind) {
		WriteImage(currentPass, rgResourceToBind);
		currentPass->AddRenderTarget(rgResourceToBind);
	}

	void RenderGraph::ReadBuffer(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToRead) {
		m_AcyclicGraph.AddReadDependency(currentPass, rgResourceToRead);
	}

	void RenderGraph::WriteBuffer(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToWrite) {
		m_AcyclicGraph.AddWriteDependency(currentPass, rgResourceToWrite);
	}

	void RenderGraph::ReadExternalBuffer(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToRead) {
		if (m_Registry.Contains(rgResourceToRead)) {
			//const auto& buffer = device->AccessResource<Buffer>(externalResources.at(resource));
			ReadBuffer(currentPass, rgResourceToRead);
			return;
		}

		//see transient image comment section
		ImportExternalResource(rgResourceToRead, RenderDeviceResourceHandle{});
		ReadBuffer(currentPass, rgResourceToRead);
	}

	void RenderGraph::WriteExternalBuffer(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToWrite) {
		if (m_Registry.Contains(rgResourceToWrite)) {
			//const auto& buffer = device->AccessResource<Buffer>(externalResources.at(resource));
			WriteBuffer(currentPass, rgResourceToWrite);
			return;
		}

		//see transient image comment section
		ImportExternalResource(rgResourceToWrite, RenderDeviceResourceHandle{});
		WriteBuffer(currentPass, rgResourceToWrite);
	}

	void RenderGraph::ReadImage(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToRead) {
		m_AcyclicGraph.AddReadDependency(currentPass, rgResourceToRead);
	}

	void RenderGraph::WriteImage(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToWrite) {
		m_AcyclicGraph.AddWriteDependency(currentPass, rgResourceToWrite);
	}

	bool RenderGraph::CheckIfPassNeedsCulling(RenderGraphPass* pass, const std::unordered_set<RenderGraphResource>& inputResources, 
		const std::unordered_set<RenderGraphResource>& outputResources) {
		LUCY_PROFILE_NEW_EVENT("RenderGraph::CheckIfPassNeedsCulling");

		const auto CheckIfPassIsDependent = [&](const std::unordered_set<RenderGraphResource>& rgResources) {
			LUCY_PROFILE_NEW_EVENT("RenderGraph::CheckIfPassIsDependent");
			for (const RenderGraphResource& rgResource : rgResources) {
				RenderGraphPass* parentPass = m_AcyclicGraph.FindOutputPassGivenResource(rgResource);
				if (parentPass && parentPass != pass && parentPass->GetCurrentState() == RenderGraphPassState::Waiting)
					return false;
			}
			return true;
		};

		const auto CheckIfExternalResourcesAreValid = [&](const std::unordered_set<RenderGraphResource>& rgResources) {
			LUCY_PROFILE_NEW_EVENT("RenderGraph::CheckIfExternalResourcesAreValid");
			for (const RenderGraphResource& rgResource : rgResources) {
				bool exists = m_Registry.Contains(rgResource);
				for (auto& handle : m_Registry.GetResourceEntry(rgResource).ResourceHandles)
					if (exists && !Renderer::IsValidRenderResource(handle)) {
						//LUCY_INFO("RenderGraph::CheckIfExternalResourcesAreValid: External resource {} is not valid, pass {} will be culled.", rgResource.GetName(), pass->GetName());
						return false;
					}
			}
			return true;
		};

		return !(CheckIfPassIsDependent(inputResources) && CheckIfExternalResourcesAreValid(inputResources));
	}
	
	RenderGraphBatches RenderGraph::CreateBatchesForRendering() const {
		LUCY_PROFILE_NEW_EVENT("RenderGraph::CreateBatchesForRendering");
		RenderGraphBatches batches;
		batches.reserve(m_AcyclicGraph.Size());

		const auto IsReadOnlyAccess = [](RenderGraphResourceAccess access) {
			switch (access) {
				case RenderGraphResourceAccess::ShaderSampledRead:
				case RenderGraphResourceAccess::TransferRead:
				case RenderGraphResourceAccess::IndexRead: 
					return true;
				default:
					return false;
			}
		};

		const auto NeedsIntraQueueBarrier = [&](RenderGraphResourceAccess src, RenderGraphResourceAccess dst) {
			if (src == RenderGraphResourceAccess::None || dst == RenderGraphResourceAccess::None)
				return false;

			// read -> read on same queue usually needs no explicit barrier here
			if (IsReadOnlyAccess(src) && IsReadOnlyAccess(dst))
				return false;

			return true;
		};

		struct LastUseInfo {
			RenderGraphPass* Pass = nullptr;
			TargetQueueFamily Queue = TargetQueueFamily::Graphics;
			RenderGraphResourceAccess Access = RenderGraphResourceAccess::None;
			RenderGraphResourceType Type = RenderGraphResourceType::Image;
			size_t BatchIndex = 0;
		};

		std::unordered_map<RenderGraphResource, LastUseInfo> lastUse;

		size_t currentBatchIndex = 0;
		const auto HandleUse = [&](RenderGraphPass* pass, const RenderGraphResourceAddInfo& use) {
			auto [it, inserted] = lastUse.try_emplace(use.Resource);

			LastUseInfo& prev = it->second;

			if (!inserted && prev.Pass) {
				if (prev.Queue != use.QueueFamily) {
					size_t srcBatchIndex = prev.BatchIndex;
					size_t dstBatchIndex = currentBatchIndex;

					batches[srcBatchIndex].OutgoingInterQueueTransitions.emplace_back(use.Resource, use.Type, prev.Queue, use.QueueFamily, prev.Access, use.Access, prev.Pass, pass);
					batches[dstBatchIndex].IncomingInterQueueTransitions.emplace_back(use.Resource, use.Type, prev.Queue, use.QueueFamily, prev.Access, use.Access, prev.Pass, pass);
				} else if (NeedsIntraQueueBarrier(prev.Access, use.Access)) {
					size_t dstBatchIndex = currentBatchIndex;
					batches[dstBatchIndex].IntraQueueTransition.emplace_back(use.Resource, use.Type, use.QueueFamily, prev.Access, use.Access, prev.Pass, pass);
				}
				//everything else is deemed to be automatically synchronized by the vulkan driver
			}

			prev = {
				.Pass = pass,
				.Queue = use.QueueFamily,
				.Access = use.Access,
				.Type = use.Type,
				.BatchIndex = currentBatchIndex
			};
		};

		// walk passes in execution order and build intra/inter queue dependencies
		TargetQueueFamily currentBatchFamily = TargetQueueFamily::Graphics;
		bool hasBatch = false;
		for (const auto& node : m_AcyclicGraph) {
			RenderGraphPass* pass = node.Pass;
			if (pass->GetCurrentState() != RenderGraphPassState::Runnable)
				continue;

			TargetQueueFamily family = pass->GetTargetQueueFamily();

			if (!hasBatch || currentBatchFamily != family) {
				currentBatchFamily = family;
				currentBatchIndex = batches.size();

				batches.emplace_back();

				hasBatch = true;
			}

			batches[currentBatchIndex].Passes.emplace_back(pass);

			for (const RenderGraphResourceAddInfo& readUse : pass->GetResourceReads())
				HandleUse(pass, readUse);
			for (const RenderGraphResourceAddInfo& writeUse : pass->GetResourceWrites())
				HandleUse(pass, writeUse);
		}

		return batches;
	}

	void RenderGraph::Update() {
		LUCY_PROFILE_NEW_EVENT("RenderGraph::Update");
		for (const auto& node : m_AcyclicGraph) {
			RenderGraphPass* pass = node.Pass;
			const auto& inputResources = node.InputResources;
			const auto& outputResources = node.OutputResources;
			
			bool needsCulling = CheckIfPassNeedsCulling(pass, inputResources, outputResources);

			if (pass->GetExecutionPolicy() == RenderGraphExecutionPolicy::Once && pass->GetCurrentState() == RenderGraphPassState::Executed) {
				pass->SetState(RenderGraphPassState::Terminated);
				continue;
			}

			if (needsCulling) {
				if (pass->GetCurrentState() == RenderGraphPassState::Waiting)
					continue;

				pass->SetState(RenderGraphPassState::Waiting);
				const auto& dependingPasses = m_AcyclicGraph.GetDependingPassesOn(pass);
				for (RenderGraphPass* dependingPass : dependingPasses)
					dependingPass->SetState(RenderGraphPassState::Waiting);
				continue;
			}

			if (pass->GetCurrentState() == RenderGraphPassState::Runnable || pass->GetCurrentState() == RenderGraphPassState::Terminated)
				continue;

			pass->SetState(RenderGraphPassState::Runnable);
			const auto& dependingPasses = m_AcyclicGraph.GetDependingPassesOn(pass);
			for (RenderGraphPass* dependingPass : dependingPasses)
				dependingPass->SetState(RenderGraphPassState::Runnable);
			//rerun the setup
			//pass->Setup();
		}
	}

	void RenderGraph::ImportFromFile(const std::filesystem::path& path) {
		//TODO: later when serialization is a thing
	}

	void RenderGraph::ExportToFile(const std::filesystem::path& path) {
		//TODO: later when serialization is a thing
	}
	
	void RenderGraph::AddPass(TargetQueueFamily targetQueueFamily, const std::string& passName, RenderGraphSetupFunc&& setupFunc) {
		LUCY_PROFILE_NEW_EVENT("RenderGraph::AddPass");
		m_Passes.try_emplace(passName, RenderGraphPassCreateInfo{
			.SetupFunc = std::move(setupFunc),
			.Registry = m_Registry,
			.TargetQueueFamily = targetQueueFamily,
			.Name = passName
		});
		auto& pass = m_Passes.at(passName);
		RenderGraphBuilder builder(this, &pass);
		pass.Setup(builder);
	}

	void RenderGraph::RemovePass(RenderGraphPass* pass) {
		m_Passes.erase(pass->GetName());
	}

	void RenderGraph::RemovePass(const std::string& passName) {
		LUCY_PROFILE_NEW_EVENT("RenderGraph::RemovePass");
		auto it = m_Passes.find(passName);
		LUCY_ASSERT(it != m_Passes.end(), "Could not find the appropriate {0} pass to remove", passName);
		(*it).second.SetState(RenderGraphPassState::Terminated);
		m_Passes.erase(it);
	}
}