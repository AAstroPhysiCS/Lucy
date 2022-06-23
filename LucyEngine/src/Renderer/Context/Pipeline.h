#pragma once

#include "../Shader/Shader.h"
#include "Renderer/Memory/Buffer/VertexBuffer.h"

#include "../Context/VulkanSwapChain.h"
#include "../Context/VulkanDevice.h"

#include "Renderer/RenderPass.h"
#include "Renderer/Memory/Buffer/FrameBuffer.h"
#include "Renderer/Memory/Buffer/UniformBuffer.h"
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

		Ref<RenderPass> RenderPass;
		Ref<FrameBuffer> FrameBuffer;
		Ref<Shader> Shader;
	};

	struct PipelineBindInfo {
		VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
		VkPipelineBindPoint PipelineBindPoint;
	};

	class OpenGLVertexBuffer;

	class Pipeline {
	public:
		Pipeline(const PipelineSpecification& specs);
		virtual ~Pipeline() = default;

		static Ref<Pipeline> Create(const PipelineSpecification& specs);

		inline Topology GetTopology() const { return m_Specs.Topology; }
		inline Rasterization GetRasterization() const { return m_Specs.Rasterization; }
		inline Ref<FrameBuffer>& GetFrameBuffer() { return m_Specs.FrameBuffer; }
		inline Ref<RenderPass>& GetRenderPass() { return m_Specs.RenderPass; }
		inline Ref<Shader>& GetShader() { return m_Specs.Shader; }

		template <class T>
		inline Ref<T> GetUniformBuffers(const uint32_t index) { return m_UniformBuffers[index].As<T>(); }
		void DestroyUniformBuffers();

		virtual void Bind(PipelineBindInfo bindInfo) = 0;
		virtual void Unbind() = 0;
		virtual void Destroy() = 0;
	protected:
		static uint32_t GetSizeFromType(ShaderDataSize size);
		static uint32_t CalculateStride(VertexShaderLayout vertexLayout);

		PipelineSpecification m_Specs;
		std::vector<Ref<UniformBuffer>> m_UniformBuffers;
	};
}