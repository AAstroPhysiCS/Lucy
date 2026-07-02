#pragma once

#include "Scene/Scene.h"

#include "RenderGraph/RenderGraph.h"

namespace Lucy {

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

		inline static constexpr const float GetNearPlaneFactor() { return s_NearPlaneFactor; }
		inline static constexpr const float GetFarPlaneFactor() { return s_FarPlaneFactor; }

		static void ResetSplit();
	private:
		const EditorCamera& m_EditorCamera;
		uint32_t m_ShadowMapSize;

		float m_CascadeSplit = 0.0f;
		float m_CascadeSplitDepth = 0.0f;

		inline static float s_LastSplitDist = 0.0f;

		inline static constexpr const float s_NearPlaneFactor = 1.0f;
		inline static constexpr const float s_FarPlaneFactor = 1.0f;
	};

	struct ShadowPass final {
		static constexpr const uint32_t NUM_CASCADES = 4;

		ShadowPass(Ref<Scene> scene, uint32_t size);
		~ShadowPass() = default;

		void AddPass(const Ref<RenderGraph>& renderGraph);

		static inline std::vector<ShadowCamera>& GetShadowCameras() { return s_ShadowCameras; }
	private:
		void InitializeShadowCameras(uint32_t size, const EditorCamera& editorCamera) const;
		
		static inline std::vector<ShadowCamera> s_ShadowCameras;

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