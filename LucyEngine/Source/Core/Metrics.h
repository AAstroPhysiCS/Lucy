#pragma once

#include <stdint.h>
#include <cstddef>

namespace Lucy {

	struct PerformanceMetrics {
		~PerformanceMetrics() = default;

		void Update();

		float DeltaTime = 0.0f;
		float FrameTime = 0.0f;
		float RenderTime = 0.0f;
		float Frames = 0.0f;

		friend struct Metrics;
	private:
		PerformanceMetrics() = default;
	};

	class MemoryTracker {
	private:
		inline static constexpr double s_ConversionValueMB = 1e6;
	public:
		~MemoryTracker() = default;

		void Update();

		///Size in megabytes
		inline constexpr double GetTotalAllocated() const { return m_TotalAllocated / s_ConversionValueMB; }

		///Size in megabytes
		inline constexpr double GetTotalFreed() const { return m_TotalFreed / s_ConversionValueMB; }

		///Size in megabytes
		inline constexpr double GetCurrentUsage() const { return m_CurrentUsage / s_ConversionValueMB; }

		friend void*	::operator new(std::size_t size);
		friend void		::operator delete(void* o, std::size_t size);

		friend struct Metrics;
	private:
		MemoryTracker() = default;

		uint64_t m_TotalAllocated = 0;
		uint64_t m_TotalFreed = 0;
		uint64_t m_CurrentUsage = 0;
	};

	struct Metrics {
		Metrics() = default;
		~Metrics() = default;

		void Update();

		PerformanceMetrics PerfMetrics;
		MemoryTracker MemTracker;
	};
}