#include "lypch.h"
#include "RenderGraph.h"
#include "RenderGraphPass.h"
#include "RenderGraphResource.h"
#include "RenderGraphBuilder.h"

#include "Renderer/ExecutionBatch.h"

#include "Renderer/Image/VulkanImage.h"

namespace Lucy {
	
	RenderGraph::RenderGraph(RenderArchitecture arch, Ref<RenderDevice> device) {
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
		const auto& batches = CreateBatchesForRendering();
		return m_Compiler->Compile(batches);
	}

	void RenderGraph::Flush() {
		LUCY_PROFILE_NEW_EVENT("RenderGraph::Flush");
		Update();
		m_Registry.Flush();
	}

	void RenderGraph::ImportExternalResource(const RenderGraphResource& rgResource, RenderResourceHandle handle) {
		m_Registry.ImportExternalResource(rgResource, handle);
	}

	void RenderGraph::ImportExternalTransientResource(const RenderGraphResource& rgResource, RenderResourceHandle handle) {
		m_Registry.ImportExternalTransientResource(rgResource, handle);
	}

	void RenderGraph::DeclareImage(const RenderGraphResource& rgResource, const ImageCreateInfo& createInfo, RenderPassLoadStoreAttachments loadStoreAccessOp) {
		DeclareImage(rgResource, createInfo, loadStoreAccessOp, UndefinedRenderGraphResource, {}, RenderPassLoadStoreAttachments::NoneNone);
	}

	void RenderGraph::DeclareImage(const RenderGraphResource& rgResource, const ImageCreateInfo& createInfo, RenderPassLoadStoreAttachments loadStoreAccessOp, const RenderGraphResource& rgResourceDepth, const ImageCreateInfo& createDepthInfo, RenderPassLoadStoreAttachments loadStoreDepthAccessOp) {
		const auto& imageHandle = Renderer::GetRenderDevice()->CreateImage(createInfo, "Image " + rgResource.GetName());
		LUCY_ASSERT(Renderer::IsValidRenderResource(imageHandle));

		m_Registry.DeclareImage(rgResource,
			imageHandle,
			RGImageData{ 
				.LoadStoreAttachment = loadStoreAccessOp, 
				.IsDepth = false 
			}
		);

		if (rgResourceDepth == UndefinedRenderGraphResource)
			return;

		const auto& imageDepthHandle = Renderer::GetRenderDevice()->CreateImage(createDepthInfo);
		LUCY_ASSERT(Renderer::IsValidRenderResource(imageDepthHandle));

		m_Registry.DeclareImage(rgResourceDepth,
			imageDepthHandle,
			RGImageData{
				.LoadStoreAttachment = loadStoreDepthAccessOp,
				.IsDepth = true
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
		ImportExternalResource(rgResourceToRead, InvalidRenderResourceHandle);
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
		ImportExternalTransientResource(rgResourceToRead, InvalidRenderResourceHandle);
		ReadImage(currentPass, rgResourceToRead);
	}

	void RenderGraph::WriteExternalImage(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToWrite) {
		if (m_Registry.Contains(rgResourceToWrite)) {
			//const auto& image = device->AccessResource<Image>(externalResources.at(resource));
			WriteImage(currentPass, rgResourceToWrite);
			return;
		}

		//see transient image comment section
		ImportExternalResource(rgResourceToWrite, InvalidRenderResourceHandle);
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
				if (m_Registry.Contains(rgResource) && !Renderer::IsValidRenderResource(m_Registry.GetResourceEntry(rgResource).ResourceHandle))
					return false;
			}
			return true;
		};

		return !(CheckIfPassIsDependent(inputResources) && CheckIfExternalResourcesAreValid(inputResources));
	}
	
	RenderGraphBatches RenderGraph::CreateBatchesForRendering() const {
		LUCY_PROFILE_NEW_EVENT("RenderGraph::CreateBatchesForRendering");
		RenderGraphBatches batches;

		std::vector<RenderGraphPass*> orderedPasses;
		orderedPasses.reserve(m_AcyclicGraph.Size());

		for (const auto& node : m_AcyclicGraph) {
			RenderGraphPass* pass = node.Pass;
			if (pass->GetCurrentState() != RenderGraphPassState::Runnable)
				continue;
			orderedPasses.push_back(pass);
		}

		if (orderedPasses.empty())
			return batches;

		// 1) Build contiguous queue-family batches
		{
			RenderGraphBatch currentBatch{};
			TargetQueueFamily currentBatchFamily = orderedPasses.front()->GetTargetQueueFamily();

			for (RenderGraphPass* pass : orderedPasses) {
				TargetQueueFamily family = pass->GetTargetQueueFamily();

				if (currentBatchFamily == family) {
					currentBatch.Passes.push_back(pass);
				} else {
					//old current batch
					batches.push_back(std::move(currentBatch));

					currentBatch = {};
					currentBatchFamily = family;

					currentBatch.Passes.push_back(pass);
				}
			}

			if (!currentBatch.Passes.empty())
				batches.push_back(std::move(currentBatch));
		}

		std::unordered_map<RenderGraphPass*, size_t> batchIndexOfPass;
		for (size_t i = 0; i < batches.size(); ++i) {
			for (RenderGraphPass* pass : batches[i].Passes)
				batchIndexOfPass[pass] = i;
		}

		const auto IsReadOnlyAccess = [](RenderGraphResourceAccess access) {
			switch (access) {
				case RenderGraphResourceAccess::ShaderSampledRead:
				case RenderGraphResourceAccess::StorageRead:
				case RenderGraphResourceAccess::TransferRead:
				case RenderGraphResourceAccess::VertexRead:
				case RenderGraphResourceAccess::IndexRead:
				case RenderGraphResourceAccess::IndirectRead:
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
		};

		std::unordered_map<RenderGraphResource, LastUseInfo> lastUse;

		const auto HandleUse = [&](RenderGraphPass* pass, const RenderGraphResourceAddInfo& use) {
			auto it = lastUse.find(use.Resource);
			if (it != lastUse.end() && it->second.Pass) {
				const LastUseInfo& prev = it->second;

				if (prev.Queue != use.QueueFamily) {
					RenderGraphInterQueueTransition tr{};
					tr.Resource = use.Resource;
					tr.ResourceType = use.Type;
					tr.SrcQueue = prev.Queue;
					tr.DstQueue = use.QueueFamily;
					tr.SrcAccess = prev.Access;
					tr.DstAccess = use.Access;
					tr.SrcPass = prev.Pass;
					tr.DstPass = pass;

					size_t srcBatchIndex = batchIndexOfPass.at(prev.Pass);
					size_t dstBatchIndex = batchIndexOfPass.at(pass);

					batches[srcBatchIndex].OutgoingInterQueueTransitions.push_back(tr);
					batches[dstBatchIndex].IncomingInterQueueTransitions.push_back(tr);
				} else if (NeedsIntraQueueBarrier(prev.Access, use.Access)) {
					RenderGraphIntraQueueTransition br{};
					br.Resource = use.Resource;
					br.ResourceType = use.Type;
					br.QueueFamily = use.QueueFamily;
					br.SrcAccess = prev.Access;
					br.DstAccess = use.Access;
					br.SrcPass = prev.Pass;
					br.DstPass = pass;

					size_t dstBatchIndex = batchIndexOfPass.at(pass);
					batches[dstBatchIndex].IntraQueueTransition.push_back(br);
				}
				//everything else is deemed to be automatically synchronized by the vulkan driver
			}

			lastUse[use.Resource] = {
				.Pass = pass,
				.Queue = use.QueueFamily,
				.Access = use.Access,
				.Type = use.Type
			};
		};

		// 3) Walk passes in execution order and build intra/inter queue dependencies
		for (RenderGraphPass* pass : orderedPasses) {
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