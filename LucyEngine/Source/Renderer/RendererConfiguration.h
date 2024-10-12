#pragma once
#include <cstdint>

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

	struct RendererConfiguration {
		RenderArchitecture RenderArchitecture = RenderArchitecture::Vulkan;
		RenderType RenderType = RenderType::Rasterizer;
		ThreadingPolicy ThreadingPolicy = ThreadingPolicy::Singlethreaded;
	};
}