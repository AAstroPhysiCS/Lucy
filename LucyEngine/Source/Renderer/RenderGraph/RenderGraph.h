#pragma once

#include <map>
#include <filesystem>

#include "Renderer/Memory/Memory.h"

#include "RenderGraphPass.h"
#include "RenderGraphCompiler.h"
#include "RenderGraphRegistry.h"
#include "DirectedAcyclicGraph.h"

namespace Lucy {

	class RenderGraph final {
	public:
		RenderGraph(RenderArchitecture arch, Ref<RenderDevice> device);
		~RenderGraph() = default;

		RenderGraph(const RenderGraph& other) = delete;
		RenderGraph(RenderGraph&& other) noexcept = delete;
		RenderGraph& operator=(const RenderGraph& other) = delete;
		RenderGraph& operator=(RenderGraph&& other) noexcept = delete;

		void Build();
		std::vector<ExecutionBatch> Execute();
		void Flush();

		void ImportFromFile(const std::filesystem::path& path);
		void ExportToFile(const std::filesystem::path& path);

		void AddPass(TargetQueueFamily targetQueueFamily, const std::string& passName, RenderGraphSetupFunc&& setupFunc);
		void RemovePass(RenderGraphPass* pass);
		void RemovePass(const std::string& passName);

		void ImportExternalResource(const RenderGraphResource& rgResource, RenderDeviceResourceHandle handle, RGResourceData data = {});
		void ImportExternalResource(const RenderGraphResource& rgResource, const std::vector<RenderDeviceResourceHandle>& handles, RGResourceData data = {});
		void ImportExternalTransientResource(const RenderGraphResource& rgResource, RenderDeviceResourceHandle handle);

		inline DirectedAcyclicGraph<RenderGraphPass, RenderGraphResource>& GetAcyclicGraph() { return m_AcyclicGraph; }
		inline size_t GetPassCount() const { return m_Passes.size(); }
	private:
		template <typename TFunc>
		inline void Traverse(TFunc&& func) {
			for (const auto& node : m_AcyclicGraph) {
				RenderGraphPass* pass = node.Pass;
				const auto& resourceReads = pass->GetResourceReads();
				const auto& resourceWrites = pass->GetResourceWrites();

				switch (node.Pass->GetCurrentState()) {
					case RenderGraphPassState::Runnable: {
						func(node.Pass);
						break;
					}
					case RenderGraphPassState::Executed:
					case RenderGraphPassState::New:
					case RenderGraphPassState::Terminated:
						LUCY_ASSERT(false, "RenderGraphPassState is new, terminated or already executed!");
						break;
				}
			}
		}
#pragma region Builder
		void DeclareImage(const RenderGraphResource& rgResource, const ImageCreateInfo& createInfo, RenderPassLoadStoreAttachments loadStoreAccessOp, bool isInFlightMode);
		void DeclareImage(const RenderGraphResource& rgResource, const ImageCreateInfo& createInfo, RenderPassLoadStoreAttachments loadStoreAccessOp,
			const RenderGraphResource& rgResourceDepth, const ImageCreateInfo& createDepthInfo, RenderPassLoadStoreAttachments loadStoreDepthAccessOp, bool isInFlightMode);
		void DeclareBuffer(const RenderGraphResource& rgResource, const RenderDeviceBufferCreateInfo& createInfo, bool isInFlightMode);

		void ReadExternalImage(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToRead);
		void ReadExternalTransientImage(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToRead);
		void WriteExternalImage(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToWrite);
		void BindRenderTarget(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToBind, const RenderGraphResource& rgResourceDepthToBind);
		void BindRenderTarget(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToBind);

		void ReadBuffer(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToRead);
		void WriteBuffer(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToWrite);
		
		void ReadExternalBuffer(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToRead);
		void WriteExternalBuffer(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToWrite);

		void ReadImage(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToRead);
		void WriteImage(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToWrite);
#pragma endregion Builder
		Ref<Image> GetImageByRGResource(const RenderGraphResource& rgResource) { return m_Registry.GetImage(rgResource); }
		Ref<Image> GetImageByRGResource(const RenderGraphResource& rgResource) const { return m_Registry.GetImage(rgResource); }

		Ref<RenderDeviceResource> GetBufferByRGResource(const RenderGraphResource& rgResource) { return m_Registry.GetBuffer(rgResource); }
		Ref<RenderDeviceResource> GetBufferByRGResource(const RenderGraphResource& rgResource) const { return m_Registry.GetBuffer(rgResource); }

		RenderPassLoadStoreAttachments GetLoadStoreAttachmentsByRGResource(const RenderGraphResource& rgResource) { return m_Registry.GetResourceEntry(rgResource).GetImageData().LoadStoreAttachment; }

		RenderDeviceResourceHandle GetHandleByRGResource(const RenderGraphResource& rgResource) { return m_Registry.GetResourceEntry(rgResource).ResourceHandles[0]; }
		std::vector<RenderDeviceResourceHandle> GetHandlesByRGResource(const RenderGraphResource& rgResource) { return m_Registry.GetResourceEntry(rgResource).ResourceHandles; }
		const std::vector<RenderDeviceResourceHandle>& GetHandlesByRGResource(const RenderGraphResource& rgResource) const { return m_Registry.GetResourceEntry(rgResource).ResourceHandles; }

		RenderGraphBatches CreateBatchesForRendering() const;

		bool CheckIfPassNeedsCulling(RenderGraphPass* pass, const std::unordered_set<RenderGraphResource>& inputResources, const std::unordered_set<RenderGraphResource>& outputResources);
		void Update();
		
		std::map<std::string, RenderGraphPass> m_Passes;
		
		DirectedAcyclicGraph<RenderGraphPass, RenderGraphResource> m_AcyclicGraph;

		Ref<RenderDevice> m_RenderDevice = nullptr;
		RenderGraphRegistry m_Registry;
		Unique<RenderGraphCompiler> m_Compiler;

		friend class RenderGraphBuilder;
		friend class VulkanRenderGraphCompiler; //for GetImageByRGResource
		friend class Renderer; //for GetImageByRGResource, GetImageData, GetBufferData
	};
}