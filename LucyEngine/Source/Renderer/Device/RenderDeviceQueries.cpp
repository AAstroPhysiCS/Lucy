#include "lypch.h"
#include "RenderDeviceQueries.h"

#include "VulkanRenderDevice.h"

namespace Lucy {
	
	Unique<RenderDeviceQuery> RenderDeviceQuery::Create(const RenderDeviceQueryCreateInfo& createInfo) {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateUnique<VulkanRenderDeviceQuery>(createInfo);
			default:
				LUCY_ASSERT(false, "No suitable API found to create the query!");
		}
		return nullptr;
	}
	
	RenderDeviceQuery::RenderDeviceQuery(const RenderDeviceQueryCreateInfo& createInfo) 
		: m_CreateInfo(createInfo) {
		LUCY_ASSERT(m_CreateInfo.Device && m_CreateInfo.QueryCount > 0, "Invalid query create info parameters!");
	}

	VulkanRenderDeviceQuery::VulkanRenderDeviceQuery(const RenderDeviceQueryCreateInfo& createInfo) 
		: RenderDeviceQuery(createInfo) {
		const auto DetermineQueryType = [](RenderDeviceQueryType type) {
			switch (type) {
				case RenderDeviceQueryType::Timestamp:
					return VK_QUERY_TYPE_TIMESTAMP;
				case RenderDeviceQueryType::Pipeline:
					return VK_QUERY_TYPE_PIPELINE_STATISTICS;
				default:
					LUCY_ASSERT(false, "Unimplemented device query type!");
			};
			return VK_QUERY_TYPE_MAX_ENUM;
		};

		//pipelineStatistics is ignored if queryType is not VK_QUERY_TYPE_PIPELINE_STATISTICS.
		VkQueryPoolCreateInfo queryPoolInfo = VulkanAPI::QueryPoolCreateInfo(GetCreateInfo().QueryCount, DetermineQueryType(GetCreateInfo().QueryType),
			VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
			VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
			VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
			VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT);

		m_QueryPools.resize(Renderer::GetMaxFramesInFlight());
		for (uint32_t i = 0; i < Renderer::GetMaxFramesInFlight(); i++)
			LUCY_VK_ASSERT(vkCreateQueryPool(GetCreateInfo().Device->As<VulkanRenderDevice>()->GetLogicalDevice(), &queryPoolInfo, nullptr, &m_QueryPools[i]));
	}

	uint32_t VulkanRenderDeviceQuery::Begin(Ref<CommandPool> cmdPool) {
		VkCommandBuffer commandBuffer = (VkCommandBuffer)cmdPool->GetCurrentFrameCommandBuffer();

		const auto BeginQuery = [&]() { vkCmdBeginQuery(commandBuffer, m_QueryPools[Renderer::GetCurrentFrameIndex()], m_ActiveQueryIndex, 0); };
		const auto BeginTimestamp = [&]() { vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, m_QueryPools[Renderer::GetCurrentFrameIndex()], m_ActiveQueryIndex); };

		switch (GetCreateInfo().QueryType) {
			case RenderDeviceQueryType::Timestamp:
				BeginTimestamp();
				break;
			case RenderDeviceQueryType::Pipeline:
				BeginQuery();
				break;
			default:
				LUCY_ASSERT(false, "Unimplemented device query type!");
		};
		return m_ActiveQueryIndex++;
	}

	uint32_t VulkanRenderDeviceQuery::End(Ref<CommandPool> cmdPool) {
		VkCommandBuffer commandBuffer = (VkCommandBuffer)cmdPool->GetCurrentFrameCommandBuffer();

		const auto EndQuery = [&]() { vkCmdEndQuery(commandBuffer, m_QueryPools[Renderer::GetCurrentFrameIndex()], m_ActiveQueryIndex); };
		const auto EndTimestamp = [&]() { vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_QueryPools[Renderer::GetCurrentFrameIndex()], m_ActiveQueryIndex); };

		switch (GetCreateInfo().QueryType) {
			case RenderDeviceQueryType::Timestamp:
				EndTimestamp();
				break;
			case RenderDeviceQueryType::Pipeline:
				EndQuery();
				break;
			default:
				LUCY_ASSERT(false, "Unimplemented device query type!");
		};
		return m_ActiveQueryIndex++;
	}

	void VulkanRenderDeviceQuery::ResetPool(Ref<CommandPool> cmdPool) {
		m_ActiveQueryIndex = 0;
		VkCommandBuffer commandBuffer = (VkCommandBuffer)cmdPool->GetCurrentFrameCommandBuffer();
		vkCmdResetQueryPool(commandBuffer, m_QueryPools[Renderer::GetCurrentFrameIndex()], 0, GetCreateInfo().QueryCount);
	}

	std::vector<uint64_t> VulkanRenderDeviceQuery::GetQueryResults() {
		uint32_t beginStageOfQuery = 0;
		uint32_t endStageOfQuery = GetCreateInfo().QueryCount;

		LUCY_ASSERT(m_ActiveQueryIndex / 2 <= endStageOfQuery, "Active device query is ongoing!");

		VkDevice logicalDevice = GetCreateInfo().Device->As<VulkanRenderDevice>()->GetLogicalDevice();

		const auto GetTimestampResults = [&]() {
			std::vector<uint64_t> timestampDatas;
			timestampDatas.resize(GetCreateInfo().QueryCount);

			vkGetQueryPoolResults(logicalDevice, m_QueryPools[Renderer::GetCurrentFrameIndex()], beginStageOfQuery, 
				endStageOfQuery - beginStageOfQuery, timestampDatas.size() * sizeof(uint64_t), timestampDatas.data(), sizeof(timestampDatas[0]), VK_QUERY_RESULT_64_BIT);
			LUCY_ASSERT(timestampDatas.size() % 2 == 0, "Querying the pool result indicated that there is a data mismatch between Begin and End functions");

			return timestampDatas;
		};

		const auto GetPipelineResults = [&]() {
			//we will have 8 pipeline statistic outputs
			static constexpr const uint32_t pipelineStatSize = 8;

			std::vector<uint64_t> pipelineStats{};
			pipelineStats.resize(pipelineStatSize);

			uint32_t dataSize = (uint32_t)(pipelineStats.size()) * sizeof(uint64_t);
			uint32_t stride = pipelineStatSize * sizeof(uint64_t);
			vkGetQueryPoolResults(logicalDevice, m_QueryPools[Renderer::GetCurrentFrameIndex()], beginStageOfQuery, 
				endStageOfQuery - beginStageOfQuery, dataSize, pipelineStats.data(), stride, VK_QUERY_RESULT_64_BIT);

			return pipelineStats;
		};

		switch (GetCreateInfo().QueryType) {
			case RenderDeviceQueryType::Timestamp:
				return GetTimestampResults();
			case RenderDeviceQueryType::Pipeline:
				return GetPipelineResults();
			default:
				LUCY_ASSERT(false, "Unimplemented device query type!");
		};
	}

	void VulkanRenderDeviceQuery::Destroy() {
		for (uint32_t i = 0; i < m_QueryPools.size(); i++)
			vkDestroyQueryPool(GetCreateInfo().Device->As<VulkanRenderDevice>()->GetLogicalDevice(), m_QueryPools[i], nullptr);
	}
}