#pragma once

#include "../Shader/Shader.h"
#include "../Buffer/VertexBuffer.h"

#include "../Context/VulkanSwapChain.h"
#include "../Context/VulkanDevice.h"

#include "Renderer/RenderPass.h"
#include "Renderer/Buffer/FrameBuffer.h"
#include "../Shader/Shader.h"

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

	struct Rasterization {
		bool DisableBackCulling = false;
		uint32_t CullingMode = 0;
		float LineWidth = 1.0f;
		PolygonMode PolygonMode = PolygonMode::FILL;
	};

	struct PipelineSpecification {
		Topology Topology = Topology::TRIANGLES;
		Rasterization Rasterization;
		VertexShaderLayout VertexShaderLayout;

		RefLucy<RenderPass> RenderPass;
		RefLucy<FrameBuffer> FrameBuffer;
		RefLucy<Shader> Shader;
	};

	class OpenGLVertexBuffer;

	class Pipeline {
	public:
		Pipeline(PipelineSpecification& specs);

		inline Topology GetTopology() const { return m_Specs.Topology; }
		inline Rasterization GetRasterization() const { return m_Specs.Rasterization; }
		inline RefLucy<FrameBuffer>& GetFrameBuffer() { return m_Specs.FrameBuffer; }
		inline RefLucy<RenderPass>& GetRenderPass() { return m_Specs.RenderPass; }

		static RefLucy<Pipeline> Create(PipelineSpecification& specs);
	protected:
		virtual void UploadVertexLayout(RefLucy<VertexBuffer>& vertexBuffer) = 0;

		PipelineSpecification m_Specs;
	};
}