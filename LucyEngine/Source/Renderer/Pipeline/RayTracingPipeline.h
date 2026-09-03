#pragma once

#include "Pipeline.h"

namespace Lucy {

	enum class AccelerationStructureType : uint8_t {
		BottomLevel,
		TopLevel
	};

	struct AccelerationStructureGeometry {
		RenderDeviceBufferReference VertexAddress{};
		RenderDeviceBufferReference IndexAddress{};
		glm::mat4 Transform = glm::mat4{ 1.0f };

		RenderDeviceSize VertexStride = 0;

		uint32_t VertexCount = 0;
		uint32_t PrimitiveCount = 0;
	};

	struct AccelerationStructureInstance {
		RenderDeviceBufferReference BottomLevelAccelerationStructureAddress = 0;

		glm::mat4 Transform = glm::mat4{ 1.0f };

		uint32_t CustomIndex = 0;
		uint32_t ShaderBindingTableRecordOffset = 0;

		uint8_t Mask = 0xFF;
	};
	
	struct BLAccelerationStructureCreateInfo {
		std::vector<AccelerationStructureGeometry> Geometries;
	};

	struct TLAccelerationStructureCreateInfo {
		std::vector<AccelerationStructureInstance> Instances;
	};

	class AccelerationStructure : public RenderDeviceResource {
	public:
		AccelerationStructure(const BLAccelerationStructureCreateInfo& createInfo, AccelerationStructureType type = AccelerationStructureType::BottomLevel);
		AccelerationStructure(const TLAccelerationStructureCreateInfo& createInfo, AccelerationStructureType type = AccelerationStructureType::TopLevel);
		virtual ~AccelerationStructure() = default;

		AccelerationStructure(const AccelerationStructure&) = delete;
		AccelerationStructure& operator=(const AccelerationStructure&) = delete;
		AccelerationStructure(AccelerationStructure&&) = delete;
		AccelerationStructure& operator=(AccelerationStructure&&) = delete;

		virtual RenderDeviceBufferReference GetDeviceAddress() const = 0;
		virtual void RTUpdate(RenderDevice* device, const TLAccelerationStructureCreateInfo& createInfo) = 0;

		const BLAccelerationStructureCreateInfo& GetBLCreateInfo() const { return m_BLCreateInfo; }
		const TLAccelerationStructureCreateInfo& GetTLCreateInfo() const { return m_TLCreateInfo; }

		AccelerationStructureType GetType() const { return m_Type; }
	private:
		BLAccelerationStructureCreateInfo m_BLCreateInfo;
		TLAccelerationStructureCreateInfo m_TLCreateInfo;

		AccelerationStructureType m_Type;
	};

	struct RayTracingPipelineCreateInfo {
		Ref<Shader> RayGenShader;
		Ref<Shader> MissShader;
		Ref<Shader> ClosestHitShader;
		Ref<Shader> AnyHitShader; //is optional
	};

	class RayTracingPipeline : public Pipeline {
	public:
		RayTracingPipeline(const RayTracingPipelineCreateInfo& createInfo);
		virtual ~RayTracingPipeline() = default;

		RayTracingPipeline(const RayTracingPipeline&) = delete;
		RayTracingPipeline& operator=(const RayTracingPipeline&) = delete;
		RayTracingPipeline(RayTracingPipeline&&) = delete;
		RayTracingPipeline& operator=(RayTracingPipeline&&) = delete;

		virtual void RTRecreate(Ref<Shader> shader) = 0;
		virtual void RTTrace(void* commandBufferHandle, uint32_t width, uint32_t height, uint32_t depth) = 0;
		virtual void RTBind(void* commandBufferHandle) = 0;

		/* stage layout ordering:
		* 0 = RayGen
		* 1 = Miss
		* 2 = ClosestHit
		* 3 = AnyHit (optional)
		*/
		Ref<Shader> GetRayGenShader() const { return GetShader(); }
		Ref<Shader> GetMissShader() const { return m_CreateInfo.MissShader; }
		Ref<Shader> GetClosestHitShader() const { return m_CreateInfo.ClosestHitShader; }
		Ref<Shader> GetAnyHitShader() const { return m_CreateInfo.AnyHitShader; }
	protected:
		const RayTracingPipelineCreateInfo& GetCreateInfo() const { return m_CreateInfo; }

		void SetCreateInfo(RayTracingPipelineCreateInfo createInfo) {
			m_CreateInfo = std::move(createInfo);
			SetShader(m_CreateInfo.RayGenShader);
		}
	private:
		RayTracingPipelineCreateInfo m_CreateInfo;
	};
}