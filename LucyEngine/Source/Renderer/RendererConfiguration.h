#pragma once
#include <cstdint>

#include "Device/RenderDeviceSceneData.h"

namespace Lucy {

	enum class RenderArchitecture : uint8_t {
		Vulkan,
		D3D12
	};

	enum class RenderType : uint8_t {
		Rasterizer,
		PathTracer
	};

	enum class ThreadingPolicy : uint8_t {
		Singlethreaded,
		Multithreaded
	};

	struct RendererSettings {
		float EnvironmentLOD = 0.0f;
		float DDGIStrength = 1.0f;

		uint32_t CullViewFlags = static_cast<uint32_t>(GPUCullViewFlags::EnableFrustumCulling) | static_cast<uint32_t>(GPUCullViewFlags::EnableConeCulling);
		bool ShowProbeSpheres = false;
	};

	struct RendererConfiguration {
		RenderArchitecture RenderArchitecture = RenderArchitecture::Vulkan;
		RenderType RenderType = RenderType::Rasterizer;
		ThreadingPolicy ThreadingPolicy = ThreadingPolicy::Singlethreaded;

		RendererSettings Settings{};
	};
}