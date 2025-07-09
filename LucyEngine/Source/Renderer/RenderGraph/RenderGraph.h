#pragma once

#include <filesystem>

#include "RenderGraphPass.h"
#include "RenderGraphRegistry.h"
#include "DirectedAcyclicGraph.h"

namespace Lucy {

	class RenderGraph final {
	public:
		RenderGraph();
		~RenderGraph() = default;

		RenderGraph(const RenderGraph& other) = delete;
		RenderGraph(RenderGraph&& other) noexcept = delete;
		RenderGraph& operator=(const RenderGraph& other) = delete;
		RenderGraph& operator=(RenderGraph&& other) noexcept = delete;

		void Compile();
		void Execute();
		void Flush();

		void ImportFromFile(const std::filesystem::path& path);
		void ExportToFile(const std::filesystem::path& path);

		void AddPass(TargetQueueFamily targetQueueFamily, const std::string& passName, RenderGraphSetupFunc&& setupFunc);
		void RemovePass(const std::string& passName);

		void ImportExternalResource(const RenderGraphResource& rgResource, RenderResourceHandle handle);
		void ImportExternalTransientResource(const RenderGraphResource& rgResource, RenderResourceHandle handle);

		inline DirectedAcyclicGraph<RenderGraphPass, RenderGraphResource>& GetAcyclicGraph() { return m_AcyclicGraph; }
		inline size_t GetPassCount() const { return m_Passes.size(); }

	private:
		template <typename TFunc>
		inline void Traverse(TFunc&& func) {
			for (const auto& node : m_AcyclicGraph) {
				switch (node.Pass->GetCurrentState()) {
					case RenderGraphPassState::Runnable:
						func(node.Pass);
						break;
					case RenderGraphPassState::New:
					case RenderGraphPassState::Terminated:
						LUCY_ASSERT(false, "RenderGraphPassState is new or terminated!");
						break;
				}
			}
		}
#pragma region Builder
		void DeclareImage(const RenderGraphResource& rgResource, const ImageCreateInfo& createInfo, RenderPassLoadStoreAttachments loadStoreAccessOp);
		void DeclareImage(const RenderGraphResource& rgResource, const ImageCreateInfo& createInfo, RenderPassLoadStoreAttachments loadStoreAccessOp,
			const RenderGraphResource& rgResourceDepth, const ImageCreateInfo& createDepthInfo, RenderPassLoadStoreAttachments loadStoreDepthAccessOp);

		void ReadExternalImage(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToRead);
		void ReadExternalTransientImage(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToRead);
		void WriteExternalImage(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToWrite);

		void BindRenderTarget(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToBind, const RenderGraphResource& rgResourceDepthToBind);
		void BindRenderTarget(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToBind);

		void ReadBuffer(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToRead);
		void WriteBuffer(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToWrite);

		void ReadImage(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToRead);
		void WriteImage(RenderGraphPass* currentPass, const RenderGraphResource& rgResourceToWrite);
#pragma endregion Builder
		inline Ref<Image> GetImageByRGResource(const RenderGraphResource& rgResource) { return m_Registry.GetImage(rgResource); }
		inline const RGImageData& GetImageData(const RenderGraphResource& rgResource) { return m_Registry.GetImageData(rgResource); }
		inline const RGBufferData& GetBufferData(const RenderGraphResource& rgResource) { return m_Registry.GetBufferData(rgResource); }

		bool CheckIfPassNeedsCulling(RenderGraphPass* pass, const std::unordered_set<RenderGraphResource>& inputResources, 
			const std::unordered_set<RenderGraphResource>& outputResources);
		void Update();
		
		std::map<std::string, RenderGraphPass> m_Passes;
		
		DirectedAcyclicGraph<RenderGraphPass, RenderGraphResource> m_AcyclicGraph;
		ExternalResources m_ExternalResources;
		ExternalResources m_ExternalTransientResources;

		RenderGraphRegistry m_Registry;

		friend class RenderGraphBuilder;
		friend class Renderer; //for GetImageByRGResource, GetImageData, GetBufferData
	};
}