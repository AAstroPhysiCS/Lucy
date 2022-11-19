#include "lypch.h"
#include "Metrics.h"

#include "Renderer/Renderer.h"

#include <chrono>

#include "GLFW/glfw3.h"

#define NOW() std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count()

namespace Lucy {

	void Metrics::Update() {
		MemTracker.Update();
		PerfMetrics.Update();
	}

	void PerformanceMetrics::Update() {
		static auto startTime = NOW();
		static float localFrames = 0;
		static float oldDeltaTime = 0;

		DeltaTime = glfwGetTime() - oldDeltaTime;
		oldDeltaTime = DeltaTime;

		localFrames++;

		if ((NOW() - startTime) > 1000) {
			startTime += 1000;

			FrameTime = 1000 / localFrames;
			Frames = localFrames;
			localFrames = 0;
		}

		RenderTime = Renderer::GetRenderTime();
	}

	void MemoryTracker::Update() {
		m_CurrentUsage = m_TotalAllocated - m_TotalFreed;
	}
}