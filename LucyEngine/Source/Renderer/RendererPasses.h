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

		inline static constexpr float GetNearPlaneFactor() { return s_NearPlaneFactor; }
		inline static constexpr float GetFarPlaneFactor() { return s_FarPlaneFactor; }

		static void ResetSplit();
	private:
		const EditorCamera& m_EditorCamera;
		uint32_t m_ShadowMapSize;

		float m_CascadeSplit = 0.0f;
		float m_CascadeSplitDepth = 0.0f;

		inline static float s_LastSplitDist = 0.0f;

		inline static constexpr float s_NearPlaneFactor = 1.0f;
		inline static constexpr float s_FarPlaneFactor = 1.0f;
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
		static inline constexpr const uint32_t HDRImageWidth = 256;
		static inline constexpr const uint32_t HDRImageHeight = 256;
#elif USE_INTEGRATED_GRAPHICS
		static inline constexpr const uint32_t HDRImageWidth = 128;
		static inline constexpr const uint32_t HDRImageHeight = 128;
#else
		static inline constexpr const uint32_t HDRImageWidth = 1024;
		static inline constexpr const uint32_t HDRImageHeight = 1024;
#endif
	private:
		Ref<Scene> m_Scene;
		uint32_t m_Width = 0;
		uint32_t m_Height = 0;
	};
#pragma endregion CubemapPass

#pragma region BRDFPass
	//TODO:
#pragma endregion BRDFPass
}