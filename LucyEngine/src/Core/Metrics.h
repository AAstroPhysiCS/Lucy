#pragma once

namespace Lucy {

	struct PerformanceMetrics {
		PerformanceMetrics() = default;
		void Update();

		float DeltaTime = 0.0f;
		float FrameTime = 0.0f;
		float Frames = 0.0f;
	};
}