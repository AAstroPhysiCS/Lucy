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
		bool DepthClampEnable = false;
		DepthCompareOp DepthCompareOp = DepthCompareOp::Less;
		float MinDepth = 0.0f;
		float MaxDepth = 1.0f;
		bool StencilTestEnable = false;
	};
}