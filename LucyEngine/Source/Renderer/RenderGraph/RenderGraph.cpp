#include "lypch.h"
#include "RenderGraph.h"
#include "RenderGraphResource.h"
#include "RenderGraphBuilder.h"

namespace Lucy {
	
	RenderGraph::RenderGraph() 
		: m_Registry(m_ExternalResources, m_ExternalTransientResources) {
	}

	void RenderGraph::Compile() {
		m_AcyclicGraph.Build();
		Update();
	}

	void RenderGraph::Execute() {
		LUCY_PROFILE_NEW_EVENT("RenderGraph::Execute");
		Traverse([](RenderGraphPass* pass) {
			Renderer::SubmitToRender(*pass);
		});
	}

	void RenderGraph::Flush() {
		LUCY_PROFILE_NEW_EVENT("RenderGraph::Flush");
		Update();
		for (auto& [rgResource, transientRenderResource] : m_ExternalTransientResources) {
			if (!Renderer::IsValidRenderResource(transientRenderResource))
				continue;
			Renderer::EnqueueResourceDestroy(transientRenderResource);
		}
	}

	void RenderGraph::ImportExternalResource(const RenderGraphResource& rgResource, RenderResourceHandle handle) {
		if (m_ExternalResources.contains(rgResource)) {
			m_ExternalResources.at(rgResource) = handle;
			return;
		}
		m_ExternalResources.try_emplace(rgResource, handle);
	}

	void RenderGraph::ImportExternalTransientResource(const RenderGraphResource& rgResource, RenderResourceHandle handle) {
		if (m_ExternalTransientResources.contains(rgResource)) {
			m_ExternalTransientResources.at(rgResource) = handle;
			return;
		}
		m_ExternalTransientResources.try_emplace(rgResource, handle);
	}

	void RenderGraph::DeclareImage(const RenderGraphResource& rgResource, const ImageCreateInfo& createInfo, RenderPassLoadStoreAttachments loadStoreAccessOp) {
		DeclareImage(rgResource, createInfo, loadStoreAccessOp, UndefinedRenderGraphResource, {}, RenderPassLoadStoreAttachments::NoneNone);
	}

	void RenderGraph::DeclareImage(const RenderGraphResource& rgResource, const ImageCreateInfo& createInfo, RenderPassLoadStoreAttachments loadStoreAccessOp, const RenderGraphResource& rgResourceDepth, const ImageCreateInfo& createDepthInfo, RenderPassLoadStoreAttachments loadStoreDepthAccessOp) {
		const auto& imageHandle = Renderer::GetRenderDevice()->CreateImage(createInfo);
		LUCY_ASSERT(Renderer::IsValidRenderResource(imageHandle));

		m_Registry.DeclareImage(rgResource,
			RGImageData{ 
				.ResourceHandle = imageHandle, 
				.LoadStoreAttachment = loadStoreAccessOp, 
				.IsDepth = false 
			}
		);

		if (rgResourceDepth == UndefinedRenderGraphResource)
			return;

		const auto& imageDepthHandle = Renderer::GetRenderDevice()->CreateImage(createDepthInfo);
		LUCY_ASSERT(Renderer::IsValidRenderResource(imageDepthHandle));

		m_Registry.DeclareImage(rgResourceDepth,
			RGImageData{
				.ResourceHandle = imageDepthHandle,
				.LoadStoreAttachment = loadStoreDepthAccessOp,
				.IsDepth = true
			}
		);
	}

	void RenderGraph::ReadExternalImage(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToRead) {
		if (auto it = m_ExternalResources.find(rgResourceToRead); it != m_ExternalResources.end()) {
			//const auto& image = device->AccessResource<Image>(externalResources.at(resource));
			ReadImage(currentPass, rgResourceToRead);
			return;
		}

		//see transient image comment section
		ImportExternalResource(rgResourceToRead, InvalidRenderResourceHandle);
		ReadImage(currentPass, rgResourceToRead);
	}

	void RenderGraph::ReadExternalTransientImage(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToRead) {
		if (auto it = m_ExternalTransientResources.find(rgResourceToRead); it != m_ExternalTransientResources.end()) {
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
		if (auto it = m_ExternalResources.find(rgResourceToWrite); it != m_ExternalResources.end()) {
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

		const auto CheckIfPassIsDependent = [this](const std::unordered_set<RenderGraphResource>& rgResources) {
			for (const RenderGraphResource& rgResource : rgResources) {
				RenderGraphPass* parentPass = m_AcyclicGraph.FindOutputPassGivenResource(rgResource);
				if (parentPass && parentPass->GetCurrentState() == RenderGraphPassState::Waiting)
					return true;
			}
			return false;
		};

		const auto CheckIfExternalResourcesAreValid = [this](const std::unordered_set<RenderGraphResource>& rgResources) {
			LUCY_PROFILE_NEW_EVENT("RenderGraph::CheckIfPassHasExternalDependency");
			for (const RenderGraphResource& rgResource : rgResources) {
				if (m_ExternalResources.contains(rgResource) && !Renderer::IsValidRenderResource(m_ExternalResources.at(rgResource)))
					return false;
				if (m_ExternalTransientResources.contains(rgResource) && !Renderer::IsValidRenderResource(m_ExternalTransientResources.at(rgResource)))
					return false;
			}
			return true;
		};

		return CheckIfPassIsDependent(inputResources) || !(CheckIfExternalResourcesAreValid(inputResources) &&
			CheckIfExternalResourcesAreValid(outputResources));
	}

	void RenderGraph::Update() {
		LUCY_PROFILE_NEW_EVENT("RenderGraph::Update");
		for (const auto& node : m_AcyclicGraph) {
			RenderGraphPass* pass = node.Pass;
			const auto& inputResources = node.InputResources;
			const auto& outputResources = node.OutputResources;
			
			bool needsCulling = CheckIfPassNeedsCulling(pass, inputResources, outputResources);

			if (needsCulling) {
				if (pass->GetCurrentState() == RenderGraphPassState::Waiting)
					continue;

				pass->SetState(RenderGraphPassState::Waiting);
				const auto& dependingPasses = m_AcyclicGraph.GetDependingPassesOn(pass);
				for (RenderGraphPass* dependingPass : dependingPasses)
					dependingPass->SetState(RenderGraphPassState::Waiting);
				continue;
			}

			if (pass->GetCurrentState() == RenderGraphPassState::Runnable)
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

	void RenderGraph::RemovePass(const std::string& passName) {
		LUCY_PROFILE_NEW_EVENT("RenderGraph::RemovePass");
		auto it = m_Passes.find(passName);
		LUCY_ASSERT(it != m_Passes.end(), "Could not find the appropriate {0} pass to remove", passName);
		(*it).second.SetState(RenderGraphPassState::Terminated);
		m_Passes.erase(it);
	}
}