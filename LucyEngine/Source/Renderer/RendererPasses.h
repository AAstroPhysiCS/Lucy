#pragma once

#include "Scene/Camera.h"

#include "Memory/Buffer/RenderDeviceBuffer.h"
#include "Device/RenderDeviceHandles.h"
#include "Device/RenderDeviceSceneData.h"

namespace Lucy {

	class Scene;
	class RenderGraph;
	class RenderGraphRegistry;

#pragma region GPUDrivenRendererPasses

	enum RenderBin : uint32_t {
		Opaque = 0,
		OpaqueDoubleSided, //TODO:
		AlphaTest, //TODO:
		AlphaTestDoubleSided, //TODO:
		Count
	};

	struct RenderDeviceGPUCullData {
		RenderDeviceBufferReference VisibleObjects = 0;
		RenderDeviceBufferReference VisibleObjectCount = 0;
		
		RenderDeviceBufferReference VisibleSubmeshes = 0;
		RenderDeviceBufferReference VisibleSubmeshCount = 0;

		RenderDeviceBufferReference SubmeshDispatchIndirect = 0;
		RenderDeviceBufferReference MeshletDispatchIndirect = 0;

		RenderDeviceBufferReference VisibleDraws = 0;
		RenderDeviceBufferReference IndirectCommands = 0;
		RenderDeviceBufferReference DrawCounts = 0;

		uint32_t ObjectCapacity = 0;
		uint32_t SubmeshCapacity = 0;
		uint32_t CommandCapacityPerBin = 0;
		uint32_t ViewIndex = 0;
		uint32_t RenderBinCount = RenderBin::Count;
	};

	struct GPUDrivenRendererPass final {
		GPUDrivenRendererPass(Ref<RenderDevice> device);
		~GPUDrivenRendererPass() = default;

		void AddPass(const Ref<RenderGraph>& renderGraph);
	private:
		static void AddHiZPass(const Ref<RenderGraph>& renderGraph);
		static void AddSubmeshDispatchBuildPass(const Ref<RenderGraph>& renderGraph);
		static void AddSubmeshCullPass(const Ref<RenderGraph>& renderGraph);
		static void AddMeshletDispatchBuildPass(const Ref<RenderGraph>& renderGraph);
		static void AddObjectCullPass(const Ref<RenderGraph>& renderGraph);
		static void AddMeshletCullPass(const Ref<RenderGraph>& renderGraph);

		static GlobalPushConstant<RenderDeviceGPUCullData> CreateGPUCullPushConstant(const RenderGraphRegistry& registry, uint32_t viewIndex);

		static inline GlobalPushConstant<RenderDeviceGPUCullData> s_GPUCullPushConstant;
	};

#pragma endregion GPUDrivenRendererPasses

#pragma region GeometryPass

	struct ForwardPBRPass final {
		ForwardPBRPass(Ref<Scene> scene, uint32_t width, uint32_t height);
		~ForwardPBRPass() = default;

		void AddPass(const Ref<RenderGraph>& renderGraph);
	private:
		Ref<Scene> m_Scene;
		uint32_t m_Width;
		uint32_t m_Height;
	};
#pragma endregion GeometryPass

#pragma region ShadowPass
	
	struct ShadowCamera : public OrthographicCamera {
		ShadowCamera(uint32_t size, const EditorCamera& editorCamera, float nearPlane, float farPlane);
		ShadowCamera(uint32_t size, const EditorCamera& editorCamera, float cascadeSplit);
		virtual ~ShadowCamera() = default;

		void UpdateView() final override;

		inline float GetCascadeSplitDepth() const { return m_CascadeSplitDepth; }

		static inline constexpr const float GetNearPlaneFactor() { return s_NearPlaneFactor; }
		static inline constexpr const float GetFarPlaneFactor() { return s_FarPlaneFactor; }

		uint32_t GetShadowMapSize() const { return m_ShadowMapSize; }
		void CreateCullView(const Ref<RenderDevice>& device);
		const RenderDeviceObjectHandle& GetCullViewHandle() const { return m_CullViewHandle; }

		static void ResetSplit();
	private:
		const EditorCamera& m_EditorCamera;
		uint32_t m_ShadowMapSize;

		float m_CascadeSplit = 0.0f;
		float m_CascadeSplitDepth = 0.0f;

		static inline float s_LastSplitDist = 0.0f;

		static inline constexpr const float s_NearPlaneFactor = 1.0f;
		static inline constexpr const float s_FarPlaneFactor = 1.0f;

		RenderDeviceObjectHandle m_CullViewHandle;
	};

	struct RenderDeviceGPUShadowCullData {
		RenderDeviceBufferReference VisibleObjects = 0;
		RenderDeviceBufferReference VisibleObjectCount = 0;

		RenderDeviceBufferReference VisibleSubmeshes = 0;
		RenderDeviceBufferReference VisibleSubmeshCount = 0;

		RenderDeviceBufferReference SubmeshDispatchIndirect = 0;
		RenderDeviceBufferReference MeshletDispatchIndirect = 0;

		RenderDeviceBufferReference VisibleDraws = 0;
		RenderDeviceBufferReference IndirectCommands = 0;
		RenderDeviceBufferReference DrawCount = 0;

		glm::uvec4 ViewIndices{0};
		glm::uvec4 Data{0}; // x = object capacity, y = command capacity, z = cascade count, w = cascade count
	};

	struct ShadowPass final {
		ShadowPass(Ref<RenderDevice> device, Ref<Scene> scene, uint32_t size);
		~ShadowPass() = default;

		void AddPass(const Ref<RenderGraph>& renderGraph);

		static inline std::vector<ShadowCamera>& GetShadowCameras() { return s_ShadowCameras; }
	private:
		static GlobalPushConstant<RenderDeviceGPUShadowCullData> CreateGPUCullPushConstant(const RenderGraphRegistry& registry);
		
		void InitializeShadowCameras(uint32_t size, const EditorCamera& editorCamera) const;

		static inline std::vector<ShadowCamera> s_ShadowCameras;
		static inline GlobalPushConstant<RenderDeviceGPUShadowCullData> s_ShadowCullPushConstant;

		Ref<RenderDevice> m_Device;
		Ref<Scene> m_Scene;
		uint32_t m_ShadowMapSize;
	};
#pragma endregion ShadowPass

#pragma region CubemapPass

	struct CubemapPass final {
		CubemapPass(Ref<Scene> scene, uint32_t width, uint32_t height);
		~CubemapPass() = default;

		void AddPass(const Ref<RenderGraph>& renderGraph);

		//the resolution of the hdr image. its an arbitrary number (increase it, if necessary)
#if USE_INTEGRATED_GRAPHICS && USE_COMPUTE_FOR_CUBEMAP_GEN
		static inline constexpr const uint32_t HDRImageSize = 256;
#elif USE_INTEGRATED_GRAPHICS
		static inline constexpr const uint32_t HDRImageSize = 128;
#else
		static inline constexpr const uint32_t HDRImageSize = 1024;
#endif
	private:
		Ref<Scene> m_Scene;
		uint32_t m_Width = 0;
		uint32_t m_Height = 0;
	};
#pragma endregion CubemapPass

#pragma region IrradiancePass

	struct IrradiancePass final {
		IrradiancePass(Ref<Scene> scene, uint32_t size);
		~IrradiancePass() = default;
		
		void AddPass(const Ref<RenderGraph>& renderGraph);
	private:
		Ref<Scene> m_Scene;
		uint32_t m_Size = 0;
	};
#pragma endregion IrradiancePass

#pragma region PrefilterPass
	
	struct PrefilterPass final {
		static constexpr inline uint32_t MAX_MIP_LEVELS = 5;

		PrefilterPass(Ref<Scene> scene, uint32_t size);
		~PrefilterPass() = default;
		
		void AddPass(const Ref<RenderGraph>& renderGraph);
	private:
		Ref<Scene> m_Scene;
		uint32_t m_CubemapSize = 0;
	};
#pragma endregion PrefilterPass

#pragma region BRDFLutPass

	struct BRDFLutPass final {
		BRDFLutPass(uint32_t size);
		~BRDFLutPass() = default;

		void AddPass(const Ref<RenderGraph>& renderGraph);
	private:
		Ref<Scene> m_Scene;
		uint32_t m_Size = 0;
	};
#pragma endregion BRDFLutPass
}