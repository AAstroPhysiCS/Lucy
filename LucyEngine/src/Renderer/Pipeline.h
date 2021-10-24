#pragma once

#include "Renderer.h"
#include "Shader/Shader.h"

namespace Lucy {

	enum class Topology {
		POINTS = 0x0000,
		TRIANGLES = 0x0004,
		LINES = 0x0001
	};

	struct PipelineSpecification
	{
		Topology Topology;

		struct Rasterization {
			bool DisableBackCulling;
			float LineWidth;
			uint32_t PolygonMode;
		} Rasterization;

		VertexShaderLayout VertexShaderLayout;
	};

	class Pipeline
	{
	public:
		static RefLucy<Pipeline>& Create(PipelineSpecification& specs);
		
		Pipeline(PipelineSpecification& specs);
	private:
		PipelineSpecification m_Specs;
	};
}