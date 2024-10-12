#pragma once

#include <unordered_set>
#include <algorithm>
#include <ranges>

namespace Lucy {

	/*
	* This is my implementation of DAG.
	* TODO: Synchronization of command queues. Lucy is currently single command queue only. Expand this, in the future
	* See more here; https://levelup.gitconnected.com/organizing-gpu-work-with-directed-acyclic-graphs-f3fd5f2c2af3
	*/

	/* TODO:
	template <typename TRenderGraphPass>
	concept Graphable = requires (TRenderGraphPass& pass) {
		pass();
		pass.GetName();

		//requires std::is_trivially_constructible_v<TRenderGraphPass>;
		//requires std::is_trivially_constructible_v<TRenderGraphResource>;
	};
	*/

	template <typename TRenderGraphPass, typename TRenderGraphResource>
	class DirectedAcyclicGraph final {
	public:
		struct Node;

		using Iterator = std::vector<Node>::iterator;
		using ConstIterator = std::vector<Node>::const_iterator;

		DirectedAcyclicGraph() = default;
		~DirectedAcyclicGraph() = default;

		void AddReadDependency(TRenderGraphPass* pass, const TRenderGraphResource& resource);
		void AddWriteDependency(TRenderGraphPass* pass, const TRenderGraphResource& resource);

		std::vector<TRenderGraphPass*> GetDependingPassesOn(TRenderGraphPass* pass);

		bool Contains(TRenderGraphPass* pass);
		bool Contains(const TRenderGraphResource& resource);

		inline TRenderGraphPass* FindOutputPassGivenResource(const TRenderGraphResource& resource) {
			auto nodeIt = std::ranges::find_if(m_Nodes, [&resource](const Node& n) {
				for (const auto& outputResourceOfNode : n.OutputResources) {
					if (outputResourceOfNode == resource)
						return true;
				}
				return false;
			});
			return nodeIt != m_Nodes.end() ? (*nodeIt).Pass : nullptr;
		}

		void Build();
	private:
		struct Node {
			TRenderGraphPass* Pass;
			uint32_t DependencyLevel = 0u;

			std::unordered_set<TRenderGraphResource> InputResources;
			std::unordered_set<TRenderGraphResource> OutputResources;

			inline bool operator==(const Node& other) const { return Pass == other.Pass; }
		};

		struct AdjacentListLink {
			AdjacentListLink() = default;
			~AdjacentListLink() = default;

			Node CurrentNode;
			bool IsVisited = false;
			std::vector<Node> InputNodes;
			std::vector<Node> OutputNodes;
		};

		using AdjacentList = std::vector<AdjacentListLink>;

		AdjacentListLink& GetLinkByNode(const Node& node) {
			auto it = std::ranges::find_if(m_AdjacentList, [&node](const AdjacentListLink& link) {
				return link.CurrentNode == node;
			});
			return *it;
		}
	public:
		inline Iterator begin() { return m_Nodes.begin(); }
		inline Iterator end() { return m_Nodes.end(); }

		inline ConstIterator begin() const { return m_Nodes.cbegin(); }
		inline ConstIterator end() const { return m_Nodes.cend(); }
	private:
		inline Iterator FindPass(TRenderGraphPass* pass) {
			return std::ranges::find_if(m_Nodes, [&pass](const Node& n) {
				return n.Pass == pass;
			});
		}

		inline Iterator FindResource(const TRenderGraphResource& resource) {
			return std::ranges::find_if(m_Nodes, [&resource](const Node& n) {
				return n.InputResources.find(resource) != n.InputResources.end() || n.OutputResources.find(resource) != n.OutputResources.end();
			});
		}

		void Compile() const;
		void BuildAdjacentList();
		void BuildTopologicalOrder();
		void Explore(AdjacentListLink& currentLink, auto& output);
		void BuildDependencyLevels();

		std::vector<Node> m_Nodes;
		AdjacentList m_AdjacentList;
	};

	template<typename TRenderGraphPass, typename TRenderGraphResource>
	inline void DirectedAcyclicGraph<TRenderGraphPass, TRenderGraphResource>::AddReadDependency(TRenderGraphPass* pass, const TRenderGraphResource& resource) {
		auto it = FindPass(pass);

		if (it == m_Nodes.end()) {
			Node n{ .Pass = pass, .InputResources = { resource } };
			m_Nodes.push_back(n);
			return;
		}

		auto& node = *it;
		node.InputResources.insert(resource);
	}

	template<typename TRenderGraphPass, typename TRenderGraphResource>
	inline void DirectedAcyclicGraph<TRenderGraphPass, TRenderGraphResource>::AddWriteDependency(TRenderGraphPass* pass, const TRenderGraphResource& resource) {
		auto it = FindPass(pass);

		if (it == m_Nodes.end()) {
			Node n{ .Pass = pass, .OutputResources = { resource } };
			m_Nodes.push_back(n);
			return;
		}

		auto& node = *it;
		node.OutputResources.insert(resource);
	}

	template<typename TRenderGraphPass, typename TRenderGraphResource>
	inline std::vector<TRenderGraphPass*> DirectedAcyclicGraph<TRenderGraphPass, TRenderGraphResource>::GetDependingPassesOn(TRenderGraphPass* passToSearchOn) {
		LUCY_ASSERT(m_AdjacentList.size() > 0, "Adjacent list is 0.");
		std::vector<TRenderGraphPass*> result;

		//find the pass to search on, on the adjacent list
		Node& nodeToSearchOn = *FindPass(passToSearchOn);
		AdjacentListLink& linkToSearchOn = GetLinkByNode(nodeToSearchOn);
		
		auto SearchOutputNode = [&](std::vector<Node>& outputNodes, auto&& SearchOutputNode) -> void {
			for (Node& outputNode : outputNodes) {
				//the outputNode.Pass uses the pass that we are searching, so add it to the result
				bool duplicateExists = std::ranges::any_of(result, [&](TRenderGraphPass* pass) {
					return outputNode.Pass == pass;
				});
				if (duplicateExists)
					continue;
				result.push_back(outputNode.Pass);

				//don't forget passes that uses this pass though, recursively iterate and add it to the vector.
				AdjacentListLink outputLinkOfThisPass = GetLinkByNode(outputNode);
				SearchOutputNode(outputLinkOfThisPass.OutputNodes, SearchOutputNode);
			}
		};

		//search all the output nodes, and don't forget passes that depend on the output passes as well.
		SearchOutputNode(linkToSearchOn.OutputNodes, SearchOutputNode);

		return result;
	}

	template<typename TRenderGraphPass, typename TRenderGraphResource>
	inline bool DirectedAcyclicGraph<TRenderGraphPass, TRenderGraphResource>::Contains(TRenderGraphPass* pass) {
		return FindPass(pass) != end();
	}

	template<typename TRenderGraphPass, typename TRenderGraphResource>
	inline bool DirectedAcyclicGraph<TRenderGraphPass, TRenderGraphResource>::Contains(const TRenderGraphResource& resource) {
		return FindResource(resource) != end();
	}

	template<typename TRenderGraphPass, typename TRenderGraphResource>
	inline void DirectedAcyclicGraph<TRenderGraphPass, TRenderGraphResource>::Build() {
		Compile();
		BuildAdjacentList();
		BuildTopologicalOrder();
		BuildDependencyLevels();
	}

	template<typename TRenderGraphPass, typename TRenderGraphResource>
	inline void DirectedAcyclicGraph<TRenderGraphPass, TRenderGraphResource>::Compile() const {
		for (const auto& node : m_Nodes) {
			LUCY_ASSERT(node.Pass->GetName().compare("Unknown"), "Handle is 0.");
			LUCY_ASSERT(node.OutputResources.size() != 0, "The render graph node '{0}' needs to have some kind of output!", node.Pass->GetName());
		}
	}

	template<typename TRenderGraphPass, typename TRenderGraphResource>
	inline void DirectedAcyclicGraph<TRenderGraphPass, TRenderGraphResource>::BuildAdjacentList() {
		m_AdjacentList.reserve(m_Nodes.size());

		//find links between nodes via input resource.
		//Note: the input/output resource of a node is the output/input resource of a another.
		for (const auto& node : m_Nodes) {
			AdjacentListLink link;
			link.CurrentNode = node;
			//Input nodes
			{
				auto searchForCommonResources = m_Nodes | std::views::filter([&node](const auto& otherNode) {
					bool isCommonResource = std::ranges::find_first_of(node.InputResources, otherNode.OutputResources, [](const auto& inputResource, const auto& outputResource) {
						return inputResource == outputResource;
					}) != node.InputResources.end();

					return isCommonResource;
				});
				link.InputNodes = std::vector(std::ranges::begin(searchForCommonResources), std::ranges::end(searchForCommonResources));
			}

			//Output nodes
			{
				auto searchForCommonResources = m_Nodes | std::views::filter([&node](const auto& otherNode) {
					bool isCommonResource = std::ranges::find_first_of(node.OutputResources, otherNode.InputResources, [](const auto& outputResource, const auto& inputResource) {
						return outputResource == inputResource;
					}) != node.OutputResources.end();

					return isCommonResource;
				});
				link.OutputNodes = std::vector(std::ranges::begin(searchForCommonResources), std::ranges::end(searchForCommonResources));
			}

			m_AdjacentList.push_back(link);
		}
	}

	template<typename TRenderGraphPass, typename TRenderGraphResource>
	inline void DirectedAcyclicGraph<TRenderGraphPass, TRenderGraphResource>::BuildTopologicalOrder() {
		LUCY_ASSERT(m_AdjacentList.size() > 0, "Adjacent list is 0.");

		std::vector<Node> sortedNodes;
		sortedNodes.reserve(m_AdjacentList.size());

		for (AdjacentListLink& link : m_AdjacentList) {
			if (link.IsVisited)
				continue;
			link.IsVisited = true;

			for (const Node& outputNodeElement : link.OutputNodes)
				Explore(GetLinkByNode(outputNodeElement), sortedNodes);
			sortedNodes.push_back(link.CurrentNode);
		}

		auto result = std::views::reverse(sortedNodes);
		m_Nodes = std::vector(std::ranges::begin(result), std::ranges::end(result));
	}

	template<typename TRenderGraphPass, typename TRenderGraphResource>
	inline void DirectedAcyclicGraph<TRenderGraphPass, TRenderGraphResource>::Explore(AdjacentListLink& currentLink, auto& output) {
		if (currentLink.IsVisited)
			return;
		currentLink.IsVisited = true;

		for (const Node& outputNodeElement : currentLink.OutputNodes)
			Explore(GetLinkByNode(outputNodeElement), output);
		output.push_back(currentLink.CurrentNode);
	}

	template<typename TRenderGraphPass, typename TRenderGraphResource>
	void DirectedAcyclicGraph<TRenderGraphPass, TRenderGraphResource>::BuildDependencyLevels() {
		LUCY_ASSERT(m_AdjacentList.size() > 0, "Adjacent list is 0.");

		for (auto& node : m_Nodes) {
			AdjacentListLink& link = GetLinkByNode(node);
			if (link.InputNodes.size() > 0) {
				node.DependencyLevel = link.InputNodes[0].DependencyLevel + 1;
				//Since we are copying all the input nodes while creating the adjacent list, we have to update the individual output nodes as well.
				for (auto& output : link.OutputNodes) {
					AdjacentListLink& outputLink = GetLinkByNode(output);
					std::ranges::for_each(outputLink.InputNodes, [&](auto& inNode) { inNode.DependencyLevel = node.DependencyLevel; });
				}
			}
		}
	}
}