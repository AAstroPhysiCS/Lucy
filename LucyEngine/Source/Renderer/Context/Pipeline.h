#pragma once

#include "../Shader/Shader.h"

#include "Renderer/RenderPass.h"
#include "Renderer/Memory/Buffer/FrameBuffer.h"
#include "Renderer/Memory/Buffer/UniformBuffer.h"
#include "Renderer/Memory/Buffer/PushConstant.h"

#include "Renderer/Descriptors/DescriptorSet.h"

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

	struct PipelineCreateInfo {
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

	class Pipeline {
	public:
		Pipeline(const PipelineCreateInfo& createInfo);
		virtual ~Pipeline() = default;

		static Ref<Pipeline> Create(const PipelineCreateInfo& createInfo);

		inline Topology GetTopology() const { return m_CreateInfo.Topology; }
		inline Rasterization GetRasterization() const { return m_CreateInfo.Rasterization; }

		inline Ref<FrameBuffer>& GetFrameBuffer() { return m_CreateInfo.FrameBuffer; }
		inline Ref<RenderPass>& GetRenderPass() { return m_CreateInfo.RenderPass; }
		inline Ref<Shader>& GetShader() { return m_CreateInfo.Shader; }

		inline const std::vector<Ref<DescriptorSet>>& GetDescriptorSets() const { return m_DescriptorSets; }

		template <class T>
		inline Ref<T> GetUniformBuffers(const char* name) {
			for (Ref<DescriptorSet> set : m_DescriptorSets) {
				const auto& uniformBuffers = set->GetAllUniformBuffers();
				for (const Ref<UniformBuffer>& ubo : uniformBuffers) {
					if (name == ubo->GetName()) {
						return ubo.As<T>();
					}
				}
			}
			LUCY_CRITICAL(fmt::format("Could not find a suitable Uniform Buffer for the given name: {0}", name));
			LUCY_ASSERT(false);
			return nullptr;
		}
		template <class T>
		inline Ref<T> GetSharedStorageBuffers(const char* name) {
			for (Ref<DescriptorSet> set : m_DescriptorSets) {
				const auto& ssbos = set->GetAllSharedStorageBuffers();
				for (const Ref<SharedStorageBuffer>& ssbo : ssbos) {
					if (name == ssbo->GetName()) {
						return ssbo.As<T>();
					}
				}
			}
			LUCY_CRITICAL(fmt::format("Could not find a suitable Uniform Buffer for the given name: {0}", name));
			LUCY_ASSERT(false);
			return nullptr;
		}

		PushConstant& GetPushConstants(const char* name);

		virtual void Bind(PipelineBindInfo bindInfo) = 0;
		virtual void Recreate(uint32_t width, uint32_t height) = 0;
		virtual void Destroy() = 0;
	protected:
		static uint32_t GetSizeFromType(ShaderDataSize size);
		static uint32_t CalculateStride(VertexShaderLayout vertexLayout);

		PipelineCreateInfo m_CreateInfo;

		std::vector<Ref<DescriptorSet>> m_DescriptorSets;
		std::vector<PushConstant> m_PushConstants;
	};
}