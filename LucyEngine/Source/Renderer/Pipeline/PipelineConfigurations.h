#pragma once

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
		bool DepthClampEnable = true;
		bool DepthClipEnable = false;
		DepthCompareOp DepthCompareOp = DepthCompareOp::Less;
		float MinDepth = 0.0f;
		float MaxDepth = 1.0f;
		bool StencilTestEnable = false;
	};

	struct BlendConfiguration {
		bool BlendEnable = true;
		VkBlendFactor SrcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		VkBlendFactor DstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		VkBlendOp ColorBlendOp = VK_BLEND_OP_ADD;
		VkBlendFactor SrcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		VkBlendFactor DstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		VkBlendOp AlphaBlendOp = VK_BLEND_OP_ADD;
	};
}