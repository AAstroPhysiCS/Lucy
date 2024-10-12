#include "lypch.h"
#include "ApplicationMetrics.h"

#include "GLFW/glfw3.h"

namespace Lucy {

	using namespace std::chrono;

#define NOW() duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count()

	void ApplicationMetrics::Update() {
		static auto startTime = NOW();
		static double localFrames = 0.0;
		static double oldDeltaTime = 0.0;

		m_DeltaTime = glfwGetTime() - oldDeltaTime;
		oldDeltaTime = m_DeltaTime;

		localFrames++;

		if ((NOW() - startTime) > 1000) {
			startTime += 1000;

			m_FrameTime = 1000 / localFrames;
			m_Frames = localFrames;
			localFrames = 0;
		}

		m_CurrentMemUsage = m_TotalMemAllocated - m_TotalMemFreed;
	}
}