#pragma once

#include "Renderer/Pipeline/PipelineConfigurations.h"
#include "Renderer/RenderPass.h"

namespace Lucy {
	
	class RenderDevice;
	class Shader;
	class CommandPool;

	class Mesh;

	class ComputePipeline;

	class RenderCommand final {
	public:
		RenderCommand(const std::string& nameOfDraw, const Ref<RenderDevice>& renderDevice, const Ref<CommandPool>& primaryCmdPool);
		~RenderCommand() = default;
		//TODO: Dynamic raster and depth configuration
#pragma region Rasterization
		constexpr void DisableBackCulling(bool backCullingDisable);
		constexpr void SetCullingMode(CullingMode cullingMode);
		constexpr void SetLineWidth(float lineWidth);
		constexpr void SetPolygonMode(PolygonMode polygonMode);

		constexpr void SetClearColor(ClearColor clearColor);
#pragma endregion Rasterization
#pragma region DepthConfiguration
		constexpr void SetDepthWriteEnable(bool depthWriteEnable);
		constexpr void SetDepthTestEnable(bool depthTestEnable);
		constexpr void SetDepthCompareOp(DepthCompareOp depthCompareOp);
		constexpr void SetStencilTestEnable(bool stencilTestEnable);
#pragma endregion DepthConfiguration
		void BindBuffers(Ref<Mesh> mesh);
		void BindBuffers(Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer);
		
		void BindPushConstant(const VulkanPushConstant& pushConstant);
		
		void BindPipeline(const Ref<GraphicsPipeline>& pipeline);
		void BindPipeline(const Ref<ComputePipeline>& pipeline);
		
		void UpdateDescriptorSets();
		void BindAllDescriptorSets();
		void BindDescriptorSet(uint32_t setIndex);
		
		void DrawIndexedMesh(Ref<Mesh> mesh, const glm::mat4& meshTransform);
		void DrawIndexedMeshWithMaterial(Ref<Mesh> mesh, const glm::mat4& meshTransform);

		void DrawMesh(Ref<Mesh> mesh);
		void DrawMeshWithPushConstant(Ref<Mesh> mesh);

		void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);
		void DispatchCompute(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

		inline double GetRenderTime(const std::vector<uint64_t>& renderTimes) const { return (double)(renderTimes[m_EndTimestampIndex] - renderTimes[m_BeginTimestampIndex]); }
		inline const std::string& GetDebugName() const { return m_DebugName; }
	private:
		void BeginSecondaryRenderCommand();
		void EndSecondaryRenderCommand();

		void BeginDebugMarker();
		void EndDebugMarker();

		void BeginTimestamp();
		void EndTimestamp();
		
		void BeginPipelineStatistics();
		void EndPipelineStatistics();

		std::string m_DebugName = "Unknown";
		Ref<RenderDevice> m_RenderDevice = nullptr;
		Ref<Shader> m_Shader = nullptr;
		Ref<CommandPool> m_PrimaryCommandPool = nullptr;

		Ref<GraphicsPipeline> m_BoundedGraphicsPipeline = nullptr;
		Ref<ComputePipeline> m_BoundedComputePipeline = nullptr;

		Rasterization m_DynamicRasterizationConfig;
		ClearColor m_DynamicClearColor;
		DepthConfiguration m_DynamicDepthConfig;

		uint32_t m_BeginTimestampIndex, m_EndTimestampIndex;

		friend class RenderCommandList;
	};
}