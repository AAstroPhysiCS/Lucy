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

	bool RenderGraph::CheckIfPassNeedsCulling(DAG::NodeID nodeId, const std::unordered_set<RenderGraphResource>& inputResources) {
		LUCY_PROFILE_NEW_EVENT("RenderGraph::CheckIfPassNeedsCulling");

		const auto CheckIfPassIsDependent = [&](DAG::NodeID nodeId, const std::unordered_set<RenderGraphResource>& rgResources) {
			LUCY_PROFILE_NEW_EVENT("RenderGraph::CheckIfPassIsDependent");
			for (auto inputNodeID : m_AcyclicGraph.GetInputNodes(nodeId)) {
				RenderGraphPass* inputPass = m_AcyclicGraph.GetPass(inputNodeID);
				if (inputPass->GetCurrentState() == RenderGraphPassState::Waiting)
					return false;
			}

			for (const RenderGraphResource& rgResource : inputResources) {
				if (!m_Registry.Contains(rgResource))
					return false;

				for (const auto& handle : m_Registry.GetResourceEntry(rgResource).ResourceHandles) {
					if (!Renderer::IsValidRenderResource(handle))
						return false;
				}
			}
			return true;
		};

		const auto CheckIfExternalResourcesAreValid = [&](const std::unordered_set<RenderGraphResource>& rgResources) {
			LUCY_PROFILE_NEW_EVENT("RenderGraph::CheckIfExternalResourcesAreValid");
			for (const RenderGraphResource& rgResource : rgResources) {
				bool exists = m_Registry.Contains(rgResource);
				for (auto& handle : m_Registry.GetResourceEntry(rgResource).ResourceHandles) {
					if (exists && !Renderer::IsValidRenderResource(handle)) {
						return false;
					}
				}
			}
			return true;
		};

		return !(CheckIfPassIsDependent(nodeId, inputResources) && CheckIfExternalResourcesAreValid(inputResources));
	}
	
	RenderGraphBatches RenderGraph::CreateBatchesForRendering() const {
		LUCY_PROFILE_NEW_EVENT("RenderGraph::CreateBatchesForRendering");

		using Graph = decltype(m_AcyclicGraph);
		using NodeID = typename Graph::NodeID;

		constexpr size_t queueCount = static_cast<size_t>(TargetQueueFamily::Count);
		constexpr uint32_t invalidSynchronizationIndex = UINT32_MAX;
		constexpr size_t invalidBatchIndex = ~0uLL;

		RenderGraphBatches batches;
		batches.reserve(m_AcyclicGraph.Size());

		const auto IsReadOnlyAccess = [](RenderGraphResourceAccess access) {
			switch (access) {
				case RenderGraphResourceAccess::TransferRead:
				case RenderGraphResourceAccess::StorageRead:
				case RenderGraphResourceAccess::VertexRead:
				case RenderGraphResourceAccess::IndexRead:
				case RenderGraphResourceAccess::IndirectRead:
				case RenderGraphResourceAccess::ShaderSampledRead:
				case RenderGraphResourceAccess::ColorAttachmentRead:
				case RenderGraphResourceAccess::DepthAttachmentRead:
					return true;
				default:
					return false;
			}
		};

		const auto NeedsIntraQueueBarrier = [&](RenderGraphResourceType type, RenderGraphResourceAccess src, RenderGraphResourceAccess dst) {
			if (src == RenderGraphResourceAccess::None || dst == RenderGraphResourceAccess::None)
				return false;

			if (!IsReadOnlyAccess(src) || !IsReadOnlyAccess(dst))
				return true;

			if (type == RenderGraphResourceType::Buffer)
				return false;

			return src != dst;
		};

		const auto CollectOnlyRunnablePasses = [&]() {
			std::vector<NodeID> passes;
			passes.reserve(m_AcyclicGraph.Size());

			for (NodeID nodeID = 0; nodeID < m_AcyclicGraph.Size(); nodeID++) {
				RenderGraphPass* pass = m_AcyclicGraph.GetPass(nodeID);
				if (pass->GetCurrentState() != RenderGraphPassState::Runnable)
					continue;

				passes.emplace_back(nodeID);
			}

			return passes;
		};

		struct PassSchedulingInfo {
			RenderGraphPass* Pass = nullptr;
			NodeID Node = Graph::InvalidNode;

			//which index has this pass in this particular queue aka. local queue index
			uint32_t QueueExecutionIndex = 0;

			/*
			 * SynchronizationIndexSet acts like a vector clock.
			 * means that reaching this node guarantees that work up to
			 * those queue-local execution indices has already completed.
			 */
			std::array<uint32_t, queueCount> SynchronizationIndexSet;

			/*
			 * Only DAG dependencies which actually require a cross-queue
			 * synchronization end up here.
			 */
			std::vector<NodeID> NodesToSyncWith;

			bool SignalRequired = false;
		};

		/*
		 * first pass:
		 * determine which passes need cross-queue synchronization.
		 */
		const auto BuildSchedulingInfos = [&](const std::vector<NodeID>& passes) {
			std::vector<PassSchedulingInfo> schedulingInfos;
			schedulingInfos.reserve(passes.size());

			std::vector<PassSchedulingInfo*> schedulingInfoByNodeID;
			schedulingInfoByNodeID.resize(m_AcyclicGraph.Size());

			std::array<uint32_t, queueCount> queueExecutionIndices{};
			std::array<PassSchedulingInfo*, queueCount> previousNodes{};

			for (NodeID nodeID : passes) {
				RenderGraphPass* pass = m_AcyclicGraph.GetPass(nodeID);
				size_t queueIndex = static_cast<size_t>(pass->GetTargetQueueFamily());

				PassSchedulingInfo& info = schedulingInfos.emplace_back();
				info.Pass = pass;
				info.Node = nodeID;
				info.SynchronizationIndexSet.fill(invalidSynchronizationIndex);
				info.NodesToSyncWith.reserve(queueCount);
				info.QueueExecutionIndex = queueExecutionIndices[queueIndex]++;
				schedulingInfoByNodeID[nodeID] = &info;

				/*
				 * vulkan queues execute submissions in queue order.
				 *
				 * therefore this pass automatically inherits everything that
				 * the previous pass on the same queue had synchronized with.
				 */
				if (previousNodes[queueIndex])
					info.SynchronizationIndexSet = previousNodes[queueIndex]->SynchronizationIndexSet;

				/*
				 * find the latest direct DAG dependency on every foreign queue.
				 *
				 * multiple predecessors on the same queue do not require
				 * multiple waits. Waiting for the latest one also guarantees
				 * completion of all earlier work on that queue.
				 */
				std::array<PassSchedulingInfo*, queueCount> closestDependencies{};

				for (NodeID dependencyNodeID : m_AcyclicGraph.GetInputNodes(nodeID)) {
					RenderGraphPass* dependencyPass = m_AcyclicGraph.GetPass(dependencyNodeID);
					if (dependencyPass->GetCurrentState() != RenderGraphPassState::Runnable)
						continue;

					PassSchedulingInfo* dependency = schedulingInfoByNodeID[dependencyNodeID];

					/*
					 * every runnable DAG predecessor must have been scheduled
					 * before this node.
					 */
					LUCY_ASSERT(dependency, "Render graph execution order violates a DAG dependency!");
					size_t dependencyQueueIndex = static_cast<size_t>(dependencyPass->GetTargetQueueFamily());

					/*
					 * same-queue dependencies need no semaphore.
					 * queue execution order already guarantees them.
					 */
					if (dependencyQueueIndex == queueIndex)
						continue;

					PassSchedulingInfo*& closestDependency = closestDependencies[dependencyQueueIndex];

					if (!closestDependency || dependency->QueueExecutionIndex > closestDependency->QueueExecutionIndex)
						closestDependency = dependency;
				}

				std::array<uint32_t, queueCount> requiredSynchronizationIndices;
				requiredSynchronizationIndices.fill(invalidSynchronizationIndex);

				size_t remainingSynchronizationCount = 0;

				for (size_t dependencyQueueIndex = 0; dependencyQueueIndex < queueCount; dependencyQueueIndex++) {
					PassSchedulingInfo* dependency = closestDependencies[dependencyQueueIndex];
					if (!dependency)
						continue;

					uint32_t requiredIndex = dependency->QueueExecutionIndex;
					uint32_t alreadySynchronizedIndex = info.SynchronizationIndexSet[dependencyQueueIndex];

					/*
					 * the previous pass on our queue may already have waited
					 * for this exact dependency, or for something newer
					 *
					 * in that case another semaphore wait would be redundant
					 */
					if (alreadySynchronizedIndex != invalidSynchronizationIndex && alreadySynchronizedIndex >= requiredIndex)
						continue;

					requiredSynchronizationIndices[dependencyQueueIndex] = requiredIndex;
					remainingSynchronizationCount++;
				}

				std::array<bool, queueCount> synchronizedQueues{};

				/*
				 * transitively select the closest dependencies on every foreign queue because its
				 * SynchronizationIndexSet contains all synchronization inherited transitively from earlier waits
				 */
				while (remainingSynchronizationCount > 0) {
					PassSchedulingInfo* bestDependency = nullptr;
					size_t bestCoverage = 0;

					for (PassSchedulingInfo* dependency : closestDependencies) {
						if (!dependency)
							continue;

						size_t coverage = 0;

						for (size_t dependencyQueueIndex = 0; dependencyQueueIndex < queueCount; dependencyQueueIndex++) {
							if (synchronizedQueues[dependencyQueueIndex])
								continue;

							uint32_t requiredIndex = requiredSynchronizationIndices[dependencyQueueIndex];
							if (requiredIndex == invalidSynchronizationIndex)
								continue;

							uint32_t synchronizedIndex = dependency->SynchronizationIndexSet[dependencyQueueIndex];
							if (synchronizedIndex != invalidSynchronizationIndex && synchronizedIndex >= requiredIndex)
								coverage++;
						}

						if (coverage > bestCoverage) {
							bestCoverage = coverage;
							bestDependency = dependency;
						}
					}

					LUCY_ASSERT(bestDependency && bestCoverage > 0, "Could not resolve render graph queue synchronization!");

					info.NodesToSyncWith.emplace_back(bestDependency->Node);

					/*
					 * the producer batch has to end directly after this pass
					 * so that Vulkan can signal its semaphore as early as possible.
					 */
					bestDependency->SignalRequired = true;

					for (size_t dependencyQueueIndex = 0; dependencyQueueIndex < queueCount; dependencyQueueIndex++) {
						uint32_t synchronizedIndex = bestDependency->SynchronizationIndexSet[dependencyQueueIndex];

						if (synchronizedIndex != invalidSynchronizationIndex) {
							uint32_t& currentIndex = info.SynchronizationIndexSet[dependencyQueueIndex];
							if (currentIndex == invalidSynchronizationIndex || synchronizedIndex > currentIndex)
								currentIndex = synchronizedIndex;
						}

						if (synchronizedQueues[dependencyQueueIndex])
							continue;

						uint32_t requiredIndex = requiredSynchronizationIndices[dependencyQueueIndex];
						if (requiredIndex == invalidSynchronizationIndex)
							continue;

						if (synchronizedIndex != invalidSynchronizationIndex && synchronizedIndex >= requiredIndex) {
							synchronizedQueues[dependencyQueueIndex] = true;
							remainingSynchronizationCount--;
						}
					}
				}

				/*
				 * Reaching this pass also means reaching this exact execution
				 * point on its own queue.
				 */
				info.SynchronizationIndexSet[queueIndex] = info.QueueExecutionIndex;
				previousNodes[queueIndex] = &info;
			}

			return schedulingInfos;
		};

		/*
		 * second pass:
		 * a new batch is required when:
		 *
		 * 1. no batch currently exists for this queue, or
		 * 2. this pass has to wait on another queue.
		 *
		 * a batch is closed when one of its passes needs to signal another
		 * queue. this allows the semaphore to be signalled as early as possible.
		 */
		const auto BuildBatches = [&](const std::vector<PassSchedulingInfo>& schedulingInfos) {
			std::array<size_t, queueCount> currentBatchIndices;
			currentBatchIndices.fill(invalidBatchIndex);

			std::vector<size_t> nodeToBatchIndex;
			nodeToBatchIndex.resize(m_AcyclicGraph.Size(), invalidBatchIndex);

			for (const PassSchedulingInfo& info : schedulingInfos) {
				RenderGraphPass* pass = info.Pass;

				size_t queueIndex = static_cast<size_t>(pass->GetTargetQueueFamily());
				size_t batchIndex = currentBatchIndices[queueIndex];

				//we create a new batch if there isnt an open batch or this pass needs to wait on another queue
				if (batchIndex == invalidBatchIndex || !info.NodesToSyncWith.empty()) {
					batchIndex = batches.size();
					currentBatchIndices[queueIndex] = batchIndex;

					RenderGraphBatch& newBatch = batches.emplace_back();
					newBatch.Dependencies.reserve(info.NodesToSyncWith.size());

					for (NodeID dependencyNodeID : info.NodesToSyncWith) {
						size_t dependencyBatchIndex = nodeToBatchIndex[dependencyNodeID];
						LUCY_ASSERT(dependencyBatchIndex != invalidBatchIndex, "Render graph synchronization dependency has no batch!");

						if (std::ranges::find(newBatch.Dependencies, dependencyBatchIndex) == newBatch.Dependencies.end())
							newBatch.Dependencies.emplace_back(dependencyBatchIndex);
					}
				}

				RenderGraphBatch& batch = batches[batchIndex];
				batch.Passes.emplace_back(pass);

				nodeToBatchIndex[info.Node] = batchIndex;

				// someone on another queue needs to wait specifically until this pass has finished, so close it immediately
				if (info.SignalRequired) {
					batch.SignalRequired = true;
					currentBatchIndices[queueIndex] = invalidBatchIndex;
				}
			}

			return nodeToBatchIndex;
		};

		/*
		 * so far we have only considered DAG dependencies to determine the execution order of passes... but thats not enough
		 * DAG edges decide:
		 *     "Does pass B have to wait for pass A?"
		 * this section decides:
		 *     "Which Vulkan access/layout/ownership transition is required?"
		 * mixing these two systems again would recreate false dependencies.
		 */
		const auto BuildResourceTransitions = [&](const std::vector<PassSchedulingInfo>& schedulingInfos, const std::vector<size_t>& nodeToBatchIndex) {
			struct LastUseInfo {
				RenderGraphPass* Pass = nullptr;

				TargetQueueFamily Queue = TargetQueueFamily::Graphics;

				RenderGraphResourceAccess Access = RenderGraphResourceAccess::None;
				RenderGraphResourceType Type = RenderGraphResourceType::Image;

				size_t BatchIndex;
			};

			std::unordered_map<RenderGraphResource, LastUseInfo> lastUse;

			const auto HandleUse = [&](RenderGraphPass* pass, size_t batchIndex, const RenderGraphResourceAddInfo& use) {
				auto [it, inserted] = lastUse.try_emplace(use.Resource);
				LastUseInfo& prev = it->second;

				if (!inserted && prev.Pass && prev.Pass != pass) {
					if (prev.Queue != use.QueueFamily) {
						batches[prev.BatchIndex].OutgoingInterQueueTransitions.emplace_back(use.Resource, use.Type, prev.Queue, use.QueueFamily, prev.Access, use.Access, prev.Pass, pass);
						batches[batchIndex].IncomingInterQueueTransitions.emplace_back(use.Resource, use.Type, prev.Queue, use.QueueFamily, prev.Access, use.Access, prev.Pass, pass);
					} else if (NeedsIntraQueueBarrier(use.Type, prev.Access, use.Access)) {
						batches[batchIndex].IntraQueueTransition.emplace_back(use.Resource, use.Type, use.QueueFamily, prev.Access, use.Access, prev.Pass, pass);
					}
				}

				prev = {
					.Pass = pass,
					.Queue = use.QueueFamily,
					.Access = use.Access,
					.Type = use.Type,
					.BatchIndex = batchIndex
				};
			};

			for (const PassSchedulingInfo& info : schedulingInfos) {
				RenderGraphPass* pass = info.Pass;
				size_t batchIndex = nodeToBatchIndex[info.Node];
				for (const RenderGraphResourceAddInfo& readUse : pass->GetResourceReads())
					HandleUse(pass, batchIndex, readUse);
				for (const RenderGraphResourceAddInfo& writeUse : pass->GetResourceWrites())
					HandleUse(pass, batchIndex, writeUse);
			}
		};

		std::vector<DirectedAcyclicGraph<RenderGraphPass, RenderGraphResource>::NodeID> passes = CollectOnlyRunnablePasses();
		if (passes.empty())
			return batches;

		std::vector<PassSchedulingInfo> schedulingInfos = BuildSchedulingInfos(passes);
		std::vector<size_t> nodeToBatchIndex = BuildBatches(schedulingInfos);
		BuildResourceTransitions(schedulingInfos, nodeToBatchIndex);

		return batches;
	}

	void RenderGraph::Update() {
		LUCY_PROFILE_NEW_EVENT("RenderGraph::Update");

		const auto SetDependingPassesState = [&](RenderGraphPass* pass, RenderGraphPassState state) {
			const auto& dependingPasses = m_AcyclicGraph.GetDependingPassesOn(pass);

			for (RenderGraphPass* dependingPass : dependingPasses) {
				if (dependingPass->GetCurrentState() == RenderGraphPassState::Terminated)
					continue;
				/*
				 * a Once pass which has already executed is effectively done
				 * will transition it to Terminated when its node is processed
				 */
				if (dependingPass->GetExecutionPolicy() == RenderGraphExecutionPolicy::Once && dependingPass->GetCurrentState() == RenderGraphPassState::Executed)
					continue;
				dependingPass->SetState(state);
			}
		};

		for (auto nodeID = 0; nodeID < m_AcyclicGraph.Size(); nodeID++) {
			RenderGraphPass* pass = m_AcyclicGraph.GetPass(nodeID);
			const auto& inputResources = m_AcyclicGraph.GetInputResources(nodeID);

			if (pass->GetCurrentState() == RenderGraphPassState::Terminated)
				continue;
			
			bool needsCulling = CheckIfPassNeedsCulling(nodeID, inputResources);

			if (pass->GetExecutionPolicy() == RenderGraphExecutionPolicy::Once && pass->GetCurrentState() == RenderGraphPassState::Executed) {
				pass->SetState(RenderGraphPassState::Terminated);
				continue;
			}

			if (needsCulling) {
				if (pass->GetCurrentState() == RenderGraphPassState::Waiting)
					continue;

				pass->SetState(RenderGraphPassState::Waiting);
				SetDependingPassesState(pass, RenderGraphPassState::Waiting);
				continue;
			}

			if (pass->GetCurrentState() == RenderGraphPassState::Runnable)
				continue;

			pass->SetState(RenderGraphPassState::Runnable);
			SetDependingPassesState(pass, RenderGraphPassState::Runnable);
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