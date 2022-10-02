#pragma once

#include "ContextPipeline.h"

#include "Renderer/RenderPass.h"
#include "Renderer/Memory/Buffer/FrameBuffer.h"
#include "Renderer/Memory/Buffer/UniformBuffer.h"

namespace Lucy {

	enum class Topology {
		POINTS,
		TRIANGLES,
		LINES
	};

	enum class PolygonMode {
		FILL,
		LINE,
		POINT
	};

	enum class CullingMode {
		None, Front, Back, FrontAndBack
	};

	struct Rasterization {
		bool DisableBackCulling = false;
		CullingMode CullingMode = CullingMode::None;
		float LineWidth = 1.0f;
		PolygonMode PolygonMode = PolygonMode::FILL;
	};

	enum class DepthCompareOp {
		Never,
		Less,
		Equal,
		LessOrEqual,
		Greater,
		NotEqual,
		GreaterOrEqual,
		Always,
	};

	struct DepthConfiguration {
		bool DepthWriteEnable = true;
		bool DepthTestEnable = true;
		DepthCompareOp DepthCompareOp = DepthCompareOp::Less;
		float MinDepth = 0.0f;
		float MaxDepth = 1.0f;
		bool StencilTestEnable = false;
	};

	struct GraphicsPipelineCreateInfo {
		Topology Topology = Topology::TRIANGLES;
		Rasterization Rasterization;
		VertexShaderLayout VertexShaderLayout;
		DepthConfiguration DepthConfiguration;

		Ref<RenderPass> RenderPass;
		Ref<FrameBuffer> FrameBuffer;
		Ref<Shader> Shader;
	};

	class GraphicsPipeline : public ContextPipeline {
	public:
		GraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo);
		virtual ~GraphicsPipeline() = default;

		inline const Ref<Shader>& GetShader() { return m_CreateInfo.Shader; }

		inline Topology GetTopology() const { return m_CreateInfo.Topology; }
		inline Rasterization GetRasterization() const { return m_CreateInfo.Rasterization; }

		inline Ref<FrameBuffer>& GetFrameBuffer() { return m_CreateInfo.FrameBuffer; }
		inline Ref<RenderPass>& GetRenderPass() { return m_CreateInfo.RenderPass; }
	protected:
		static uint32_t GetSizeFromType(ShaderDataSize size);
		static uint32_t CalculateStride(VertexShaderLayout vertexLayout);

		GraphicsPipelineCreateInfo m_CreateInfo;
	};
}