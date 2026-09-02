#pragma once

#include <unordered_set>
#include <algorithm>
#include <ranges>
#include <deque>

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
		using NodeID = uint32_t;

		static constexpr NodeID InvalidNode = UINT32_MAX;

		struct Node;

		using Iterator = std::vector<Node>::iterator;
		using ConstIterator = std::vector<Node>::const_iterator;

		DirectedAcyclicGraph() = default;
		~DirectedAcyclicGraph() = default;

		DirectedAcyclicGraph(const DirectedAcyclicGraph& other) = delete;
		DirectedAcyclicGraph(DirectedAcyclicGraph&& other) noexcept = delete;
		DirectedAcyclicGraph& operator=(const DirectedAcyclicGraph& other) = delete;
		DirectedAcyclicGraph& operator=(DirectedAcyclicGraph&& other) noexcept = delete;

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
		TRenderGraphPass* GetPass(NodeID nodeID) const { return m_Nodes[nodeID].Pass; }

		const std::vector<NodeID>& GetInputNodes(NodeID nodeID) const { return m_AdjacentList[nodeID].InputNodes; }
		const std::vector<NodeID>& GetOutputNodes(NodeID nodeID) const { return m_AdjacentList[nodeID].OutputNodes; }

		const std::unordered_set<TRenderGraphResource>& GetInputResources(NodeID nodeID) const { return m_Nodes[nodeID].InputResources; }
		const std::unordered_set<TRenderGraphResource>& GetOutputResources(NodeID nodeID) const { return m_Nodes[nodeID].OutputResources; }

		uint32_t GetOriginalIndex(NodeID nodeID) const { return m_Nodes[nodeID].OriginalIndex; }
		uint32_t GetDependencyLevel(NodeID nodeID) const { return m_Nodes[nodeID].DependencyLevel; }
		uint32_t GetCriticalPathLength(NodeID nodeID) const { return m_Nodes[nodeID].CriticalPathLength; }

		size_t Size() const { return m_Nodes.size(); }

		void Build();
	private:
		struct Node {
			TRenderGraphPass* Pass;
			uint32_t DependencyLevel = 0u;
			uint32_t CriticalPathLength = 1u;
			uint32_t OriginalIndex = 0u;

			std::unordered_set<TRenderGraphResource> InputResources;
			std::unordered_set<TRenderGraphResource> OutputResources;

			inline bool operator==(const Node& other) const { return Pass == other.Pass; }
		};

		struct AdjacentListLink {
			std::vector<NodeID> InputNodes;
			std::vector<NodeID> OutputNodes;
		};

		using AdjacentList = std::vector<AdjacentListLink>;

		AdjacentListLink& GetLinkByNode(const Node& node) {
			auto it = std::ranges::find_if(m_AdjacentList, [&node](const AdjacentListLink& link) {
				return link.CurrentNode == node;
			});
			return *it;
		}
	public:
		Iterator begin() { return m_Nodes.begin(); }
		Iterator end() { return m_Nodes.end(); }

		ConstIterator begin() const { return m_Nodes.cbegin(); }
		ConstIterator end() const { return m_Nodes.cend(); }

		Iterator FindPass(TRenderGraphPass* pass) {
			return std::ranges::find_if(m_Nodes, [&pass](const Node& n) {
				return n.Pass == pass;
			});
		}

		const Node& operator[](ConstIterator it) const {
			LUCY_ASSERT(it != m_Nodes.end(), "Iterator is out of bounds!");
			return *it;
		}
	private:
		Iterator FindResource(const TRenderGraphResource& resource) {
			return std::ranges::find_if(m_Nodes, [&resource](const Node& n) {
				return n.InputResources.find(resource) != n.InputResources.end() || n.OutputResources.find(resource) != n.OutputResources.end();
			});
		}

		NodeID FindNodeID(TRenderGraphPass* pass) const {
			for (NodeID nodeID = 0; nodeID < m_Nodes.size(); nodeID++) {
				if (m_Nodes[nodeID].Pass == pass)
					return nodeID;
			}

			return InvalidNode;
		}

		void Compile() const;

		void BuildAdjacentList();
		void BuildTopologicalOrder();
		void BuildDependencyLevels();

		void ConnectNodes(NodeID sourceNodeID, NodeID destinationNodeID);

		std::vector<Node> m_Nodes;
		AdjacentList m_AdjacentList;
	};

	template<typename TRenderGraphPass, typename TRenderGraphResource>
	inline void DirectedAcyclicGraph<TRenderGraphPass, TRenderGraphResource>::AddReadDependency(TRenderGraphPass* pass, const TRenderGraphResource& resource) {
		auto it = FindPass(pass);

		if (it == m_Nodes.end()) {
			Node n{ .Pass = pass, .OriginalIndex = static_cast<uint32_t>(m_Nodes.size()), .InputResources = { resource } };
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
			Node n{ .Pass = pass, .OriginalIndex = static_cast<uint32_t>(m_Nodes.size()), .OutputResources = { resource } };
			m_Nodes.push_back(n);
			return;
		}

		auto& node = *it;
		node.OutputResources.insert(resource);
	}

	template<typename TRenderGraphPass, typename TRenderGraphResource>
	inline std::vector<TRenderGraphPass*> DirectedAcyclicGraph<TRenderGraphPass, TRenderGraphResource>::GetDependingPassesOn(TRenderGraphPass* passToSearchOn) {
		LUCY_PROFILE_NEW_EVENT("DirectedAcyclicGraph::GetDependingPassesOn");
		LUCY_ASSERT(m_AdjacentList.size() > 0, "Adjacent list is 0.");
		std::vector<TRenderGraphPass*> result;

		NodeID startNodeID = FindNodeID(passToSearchOn);
		LUCY_ASSERT(startNodeID != InvalidNode);

		std::vector<bool> visited;
		visited.resize(m_Nodes.size());

		std::vector<NodeID> nodesToVisit = m_AdjacentList[startNodeID].OutputNodes;

		while (!nodesToVisit.empty()) {
			NodeID nodeID = nodesToVisit.back();
			nodesToVisit.pop_back();

			if (visited[nodeID])
				continue;

			visited[nodeID] = true;

			result.emplace_back(m_Nodes[nodeID].Pass);

			for (NodeID outputNodeID : m_AdjacentList[nodeID].OutputNodes) {
				if (!visited[outputNodeID])
					nodesToVisit.emplace_back(outputNodeID);
			}
		}

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
		LUCY_PROFILE_NEW_EVENT("DirectedAcyclicGraph::Build");
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
		LUCY_PROFILE_NEW_EVENT("DirectedAcyclicGraph::BuildAdjacentList");

		struct ResourceState {
			// the most recent pass that wrote this resource
			NodeID LastWriter = InvalidNode;
			// all passes that have read it since that last write
			std::vector<NodeID> Readers;
		};

		m_AdjacentList.resize(m_Nodes.size());

		std::unordered_map<TRenderGraphResource, ResourceState> resourceStates;
		resourceStates.reserve(m_Nodes.size() * 2);

		for (NodeID nodeID = 0; nodeID < m_Nodes.size(); nodeID++) {
			Node& node = m_Nodes[nodeID];

			// RAW
			for (const TRenderGraphResource& resource : node.InputResources) {
				ResourceState& state = resourceStates[resource];
				ConnectNodes(state.LastWriter, nodeID);

				if (std::ranges::find(state.Readers, nodeID) == state.Readers.end())
					state.Readers.emplace_back(nodeID);
			}

			// WAW and WAR
			for (const TRenderGraphResource& resource : node.OutputResources) {
				ResourceState& state = resourceStates[resource];
				ConnectNodes(state.LastWriter, nodeID);
				for (NodeID readerNodeID : state.Readers)
					ConnectNodes(readerNodeID, nodeID);
				state.Readers.clear();
				state.LastWriter = nodeID;
			}
		}
	}

	/*
	* using the kahn's algorithm to build a topological order of the nodes in the DAG (the medium article does use a different algorithm)
	*/
	template<typename TRenderGraphPass, typename TRenderGraphResource>
	inline void DirectedAcyclicGraph<TRenderGraphPass, TRenderGraphResource>::BuildTopologicalOrder() {
		LUCY_PROFILE_NEW_EVENT("DirectedAcyclicGraph::BuildTopologicalOrder");

		if (m_Nodes.empty())
			return;

		std::vector<uint32_t> remainingDependencies;
		remainingDependencies.reserve(m_Nodes.size());
		for (const AdjacentListLink& link : m_AdjacentList)
			remainingDependencies.emplace_back(static_cast<uint32_t>(link.InputNodes.size()));

		// ready nodes are those with no remaining dependencies, kahn's algorithm priorities those nodes first
		// we want to distinguish between passes that couldn't be done... so in that case, the originalIndex (aka. the order of insertion is taken in to consideration)
		std::deque<NodeID> readyNodes;

		for (NodeID nodeID = 0; nodeID < m_Nodes.size(); nodeID++) {
			if (remainingDependencies[nodeID] != 0)
				continue;
			readyNodes.emplace_back(nodeID);
		}

		std::vector<NodeID> topologicalOrder;
		topologicalOrder.reserve(m_Nodes.size());

		while (!readyNodes.empty()) {
			NodeID nodeID = readyNodes.front();
			readyNodes.pop_front();

			topologicalOrder.emplace_back(nodeID);

			for (NodeID outputNodeID : m_AdjacentList[nodeID].OutputNodes) {
				uint32_t& dependencyCount = remainingDependencies[outputNodeID];
				LUCY_ASSERT(dependencyCount > 0);

				dependencyCount--;
				if (dependencyCount != 0)
					continue;

				readyNodes.emplace_back(outputNodeID);
			}
		}

		LUCY_ASSERT(topologicalOrder.size() == m_Nodes.size(), "Directed graph contains a cyclic dependency!");

		std::vector<NodeID> newNodeIDs;
		newNodeIDs.resize(m_Nodes.size());
		for (NodeID newNodeID = 0; newNodeID < topologicalOrder.size(); newNodeID++) {
			NodeID oldNodeID = topologicalOrder[newNodeID];
			newNodeIDs[oldNodeID] = newNodeID;
		}

		std::vector<Node> sortedNodes;
		sortedNodes.reserve(m_Nodes.size());
		for (NodeID oldNodeID : topologicalOrder)
			sortedNodes.emplace_back(std::move(m_Nodes[oldNodeID]));

		AdjacentList sortedAdjacentList;
		sortedAdjacentList.resize(m_AdjacentList.size());

		for (NodeID newNodeID = 0; newNodeID < topologicalOrder.size(); newNodeID++) {
			NodeID oldNodeID = topologicalOrder[newNodeID];

			const AdjacentListLink& oldLink = m_AdjacentList[oldNodeID];

			AdjacentListLink& newLink = sortedAdjacentList[newNodeID];
			newLink.InputNodes.reserve(oldLink.InputNodes.size());
			newLink.OutputNodes.reserve(oldLink.OutputNodes.size());

			for (NodeID oldInputNodeID : oldLink.InputNodes)
				newLink.InputNodes.emplace_back(newNodeIDs[oldInputNodeID]);
			for (NodeID oldOutputNodeID : oldLink.OutputNodes)
				newLink.OutputNodes.emplace_back(newNodeIDs[oldOutputNodeID]);
		}

		//both are sorted... so both can be used in conjunction with other stuff
		m_Nodes = std::move(sortedNodes);
		m_AdjacentList = std::move(sortedAdjacentList);
	}

	template<typename TRenderGraphPass, typename TRenderGraphResource>
	void DirectedAcyclicGraph<TRenderGraphPass, TRenderGraphResource>::BuildDependencyLevels() {
		LUCY_PROFILE_NEW_EVENT("DirectedAcyclicGraph::BuildDependencyLevels"); 
		LUCY_ASSERT(m_AdjacentList.size() > 0, "Adjacent list is 0.");

		//longest path from root
		for (NodeID nodeID = 0; nodeID < m_Nodes.size(); nodeID++) {
			Node& node = m_Nodes[nodeID];
			node.DependencyLevel = 0;
			for (NodeID inputNodeID : m_AdjacentList[nodeID].InputNodes)
				node.DependencyLevel = std::max(node.DependencyLevel, m_Nodes[inputNodeID].DependencyLevel + 1);
		}

		//longest path to leaf
		for (NodeID nodeID = static_cast<NodeID>(m_Nodes.size()); nodeID-- > 0;) {
			Node& node = m_Nodes[nodeID];
			node.CriticalPathLength = 1;
			for (NodeID outputNodeID : m_AdjacentList[nodeID].OutputNodes)
				node.CriticalPathLength = std::max(node.CriticalPathLength, m_Nodes[outputNodeID].CriticalPathLength + 1);
		}
	}

	template<typename TRenderGraphPass, typename TRenderGraphResource>
	inline void DirectedAcyclicGraph<TRenderGraphPass, TRenderGraphResource>::ConnectNodes(NodeID sourceNodeID, NodeID destinationNodeID) {
		if (sourceNodeID == InvalidNode || sourceNodeID == destinationNodeID) {
			return;
		}
		
		auto& outputNodes = m_AdjacentList[sourceNodeID].OutputNodes;

		/*
		* this check is to avoid duplicates... aka distinct edges are only allowed
		*/
		if (std::ranges::find(outputNodes, destinationNodeID) != outputNodes.end())
			return;

		outputNodes.emplace_back(destinationNodeID);

		m_AdjacentList[destinationNodeID].InputNodes.emplace_back(sourceNodeID);
	}
}