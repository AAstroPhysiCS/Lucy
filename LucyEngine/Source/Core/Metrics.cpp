#include "lypch.h"
#include "Metrics.h"

#include "Renderer/Renderer.h"

#include <chrono>

#include "GLFW/glfw3.h"

using namespace std::chrono;

#define NOW() duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count()

namespace Lucy {

	void Metrics::Update() {
		MemTracker.Update();
		PerfMetrics.Update();
	}

	void PerformanceMetrics::Update() {
		static auto startTime = NOW();
		static double localFrames = 0.0;
		static double oldDeltaTime = 0.0;

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