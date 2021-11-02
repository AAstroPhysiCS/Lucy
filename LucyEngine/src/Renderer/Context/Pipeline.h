#pragma once

#include "../Renderer.h"
#include "../Shader/Shader.h"

namespace Lucy {

	enum class Topology {
		POINTS,
		TRIANGLES,
		LINES
	};

	struct Rasterization {
		bool DisableBackCulling = false;
		float LineWidth = 1.0f;
		uint32_t PolygonMode;
	};

	struct PipelineSpecification
	{
		Topology Topology = Topology::TRIANGLES;
		Rasterization Rasterization;
		VertexShaderLayout VertexShaderLayout;
	};

	class OpenGLVertexBuffer;

	class Pipeline
	{
	public:
		Pipeline(PipelineSpecification& specs);

		inline Topology GetTopology() const { return m_Specs.Topology; }
		inline Rasterization GetRasterization() const { return m_Specs.Rasterization; }
		
		static RefLucy<Pipeline> Create(PipelineSpecification& specs);
	protected:
		virtual void UploadVertexLayout(RefLucy<OpenGLVertexBuffer>& vertexBuffer) = 0;
		
		PipelineSpecification m_Specs;
	};
}